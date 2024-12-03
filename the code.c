#include <PulseSensorPlayground.h>
#include <SoftwareSerial.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// these are the libraries, so the pulseplayground works with the pulse sensor,
// the softwareserial helps with the communication with the Bluetooth module,
// and the others are just C language libraries

// ok so these are like the constants for the pins and all, 
// they help define what pins are for what hardware, 
// and some thresholds for heart rate
#define PULSE_SENSOR_PIN A0
#define HEARTBEAT_LED_PIN 13
#define BLUETOOTH_RX_PIN 2
#define BLUETOOTH_TX_PIN 3
#define DEFAULT_SIGNAL_THRESHOLD 550
#define DEFAULT_MAX_HEART_RATE 120   // if heart rate goes above this, it’s too high
#define DEFAULT_MIN_HEART_RATE 40         // if heart rate is below this, it's too low
#define NO_FINGER_SIGNAL_THRESHOLD 50     // basically when no finger is on the sensor
#define BLUETOOTH_BUFFER_SIZE 64
#define REPORT_INTERVAL_MS 60000  // this time is in milliseconds, used to generate session reports

// so yeah these macros are basically shortcuts to log messages in the code
// like instead of writing the full logMessage function every time, 
// you can just use these and pass the message, and it'll tag the log 
// with the right type (INFO, DEBUG, ERROR, etc.)
// super useful to keep the code cleaner and easier to read
#define LOG_INFO(msg) logMessage("INFO", msg)
#define LOG_DEBUG(msg) logMessage("DEBUG", msg)
#define LOG_ERROR(msg) logMessage("ERROR", msg)
#define LOG_DATA(msg) logMessage("DATA", msg)
#define LOG_WARNING(msg) logMessage("WARNING", msg)
#define LOG_REPORT(msg) logMessage("REPORT", msg)

// this struct is for tracking the heart rate session stats, like it stores
// everything you need to summarize a session
typedef struct {
    int totalHeartbeats;
    int totalBPM;
    int bpmReadingsCount;
    unsigned long sessionStartTime;
} HeartRateSession;

// this sets up the Bluetooth module and pulse sensor
SoftwareSerial Bluetooth(BLUETOOTH_RX_PIN, BLUETOOTH_TX_PIN);  // this is the Bluetooth communication
PulseSensorPlayground pulseSensor;
volatile bool isFingerDetected = false;  
HeartRateSession currentSession = {0, 0, 0, 0};
char bluetoothBuffer[BLUETOOTH_BUFFER_SIZE];
int bufferIndex = 0;   // this stores the incoming Bluetooth commands

// this one below basically declares all the functions that are being used in the code
void initializeSystem();
void logMessage(const char *level, const char *message);
void processBluetoothCommand(const char *cmd);
void updateHeartRate(int bpm);
void reportSessionStats();
void sendHeartRateRecommendation(float avgBPM);
bool isFingerOnSensor(int amplitude);

// okay this one calculates the average beats per minute so after the session it displays the average 
inline float calculateAverageBPM() {
    return currentSession.bpmReadingsCount > 0
               ? (float)currentSession.totalBPM / currentSession.bpmReadingsCount  // normal calc
               : 0.0f;   // if no readings, just return 0
}

// the setup function runs once when the system starts, so this is where
// we set everything up, like the sensor, Bluetooth, and initialize stuff
void setup() {
    initializeSystem();
    currentSession.sessionStartTime = millis();
    LOG_INFO("Setup complete. Ready to track heart rate.");
}

// this is the main loop, it keeps running forever
// it does stuff like checking if there's a finger, logging data, and processing commands
void loop() {
    int amplitude = pulseSensor.getLatestSample();
    bool fingerDetectedNow = isFingerOnSensor(amplitude); // is there a finger?

    // checks if finger detection changed (added or removed)
    if (fingerDetectedNow != isFingerDetected) {
        isFingerDetected = fingerDetectedNow;

        if (isFingerDetected) {
            LOG_INFO("Finger detected! Tracking heart rate.");
            currentSession.sessionStartTime = millis();
        } else {
            LOG_INFO("Finger removed. Generating session report.");
            reportSessionStats();
        }
    }

    // If finger is on sensor, process heart rate
    if (isFingerDetected) {
        int bpm = pulseSensor.getBeatsPerMinute();

        if (pulseSensor.sawStartOfBeat() && bpm >= 30 && bpm <= 200) {
            updateHeartRate(bpm);
        }

        // Generate periodic reports
        if (millis() - currentSession.sessionStartTime >= REPORT_INTERVAL_MS) {
            reportSessionStats();
        }
    }

    // Process incoming Bluetooth commands
    while (Bluetooth.available()) {
        char c = Bluetooth.read();
        if (c == '\n' || bufferIndex >= BLUETOOTH_BUFFER_SIZE - 1) {
            bluetoothBuffer[bufferIndex] = '\0';
            processBluetoothCommand(bluetoothBuffer);
            bufferIndex = 0; // Reset buffer
        } else {
            bluetoothBuffer[bufferIndex++] = c;
        }
    }
}

// okay this initializes the serial and the Bluetooth interfaces, so it basically checks if the pulse sensor is working or not
// if it's not then it logs an error
void initializeSystem() {
    Serial.begin(9600);
    Bluetooth.begin(9600);

    LOG_DEBUG("Initializing system...");

    pulseSensor.analogInput(PULSE_SENSOR_PIN);
    pulseSensor.blinkOnPulse(HEARTBEAT_LED_PIN);
    pulseSensor.setThreshold(DEFAULT_SIGNAL_THRESHOLD);

    if (!pulseSensor.begin()) {
        LOG_ERROR("Pulse sensor initialization failed. Halting.");
        while (1); // Stop everything
    }

    LOG_INFO("Pulse sensor initialized successfully.");
}

// this is the message that logs to both the serial monitor and the Bluetooth terminal on the phone
void logMessage(const char *level, const char *msg) {
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "[%lu ms] [%s] %s", millis(), level, msg);
    Serial.println(buffer);
    Bluetooth.println(buffer);
}

// this handles incoming Bluetooth commands, like setting thresholds and showing help
void processBluetoothCommand(const char *cmd) {
    LOG_DEBUG("Processing Bluetooth command...");
    if (strncmp(cmd, "SET_THRESHOLD:", 14) == 0) {
        int newThreshold = atoi(cmd + 14);
        if (newThreshold >= 100 && newThreshold <= 1000) {
            pulseSensor.setThreshold(newThreshold);
            LOG_INFO("Threshold updated.");
        } else {
            LOG_WARNING("Threshold out of range!");
        }
    } else if (strcmp(cmd, "HELP") == 0) {
        LOG_INFO("Commands: SET_THRESHOLD:<value>, HELP");
    } else {
        LOG_WARNING("Unknown command received.");
    }
}

// this updates the heart rate stats, so like every time it gets a reading, 
// it increments the heartbeats and updates the BPM
void updateHeartRate(int bpm) {
    currentSession.totalHeartbeats++;
    currentSession.totalBPM += bpm;
    currentSession.bpmReadingsCount++;
    char logMsg[64];
    snprintf(logMsg, sizeof(logMsg), "Heartbeat recorded: BPM = %d", bpm);
    LOG_DATA(logMsg);
}

// this just checks if a finger is on the sensor
bool isFingerOnSensor(int amplitude) {
    return amplitude > NO_FINGER_SIGNAL_THRESHOLD;
}

// this generates the session stats after the session ends and resets for the next session
void reportSessionStats() {
    float avgBPM = calculateAverageBPM();

    if (currentSession.bpmReadingsCount > 0) { // if there are valid readings
        char stats[128];
        snprintf(stats, sizeof(stats), "Session complete. Avg BPM: %.2f, Total Beats: %d",
                 avgBPM, currentSession.totalHeartbeats);
        LOG_REPORT(stats);
        sendHeartRateRecommendation(avgBPM);
    } else {
        LOG_WARNING("No valid readings to report.");
    }

    // this resets the data for the next session
    currentSession = (HeartRateSession){0, 0, 0, millis()};
}

// this just basically sends recommendations based on the heart readings,
// so like if it reaches max relax, average, or low
void sendHeartRateRecommendation(float avgBPM) {
    if (avgBPM > DEFAULT_MAX_HEART_RATE) {
        LOG_INFO("Relax and breathe deeply to lower your heart rate.");
    } else if (avgBPM < DEFAULT_MIN_HEART_RATE) {
        LOG_INFO("Try light exercise to raise your heart rate.");
    } else {
        LOG_INFO("Heart rate is within a healthy range. Keep it up!");
    }
}

