// these are the libarries, so the pulseplayground it works with the pulse sensor, the softwareserial helps with the communication with the bluetootj module, and the others are just C language libraries
#include <PulseSensorPlayground.h>
#include <SoftwareSerial.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// these are the constants so luike the values for the pins, thresholds and the other stuff
#define PULSE_SENSOR_PIN A0
#define HEARTBEAT_LED_PIN 13
#define BLUETOOTH_RX_PIN 2
#define BLUETOOTH_TX_PIN 3
#define DEFAULT_SIGNAL_THRESHOLD 550
#define DEFAULT_MAX_HEART_RATE 120
#define DEFAULT_MIN_HEART_RATE 40
#define NO_FINGER_SIGNAL_THRESHOLD 50
#define BLUETOOTH_BUFFER_SIZE 64
#define REPORT_INTERVAL_MS 60000  // this time is in miliseconds i did thi in otder to generate the sesion reports so like after the reading there will be a report that shows you BPM and other stuff

// these are predefined macro for different log levels so what does this mean well it means that 
#define LOG_INFO(msg) logMessage("INFO", msg)
#define LOG_DEBUG(msg) logMessage("DEBUG", msg)
#define LOG_ERROR(msg) logMessage("ERROR", msg)
#define LOG_DATA(msg) logMessage("DATA", msg)
#define LOG_REPORT(msg) logMessage("REPORT", msg)
#define LOG_WARNING(msg) logMessage("WARNING", msg)

// this bascially tracks the sesion statiscits so like the total heartbeats, total BPM and the sesion of the start time
typedef struct {
    int totalHeartbeats;
    int totalBPM;
    int bpmReadingsCount;
    unsigned long sessionStartTime;
} HeartRateSession;


SoftwareSerial Bluetooth(BLUETOOTH_RX_PIN, BLUETOOTH_TX_PIN);  //this is the bluetooth communicatio
PulseSensorPlayground pulseSensor;
volatile bool isFingerDetected = false;  
char bluetoothBuffer[BLUETOOTH_BUFFER_SIZE];
int bluetoothBufferIndex = 0;   //this stores the incoming bluetooth commands
HeartRateSession currentSession = {0, 0, 0, 0};

// this one below basically declares all the fucntoins that are being used in the code
void initializeSystem();
void logMessage(const char *level, const char *message);
void processBluetoothCommand(const char *command);
void updateHeartRate(int bpm);
void reportSessionStatistics();
void sendRecommendation(float averageBPM);
bool checkFingerPlacement(int signalAmplitude);

// okay this one calculates the average beats per minute so after the sesion to display the average 
inline float calculateAverageBPM() {
    return currentSession.bpmReadingsCount > 0
               ? (float)currentSession.totalBPM / currentSession.bpmReadingsCount
               : 0.0f;  
}

void setup() {
    initializeSystem();
    currentSession.sessionStartTime = millis();
    LOG_DEBUG("Setup completed successfully.");
}

// this si the main loop, so it checks if the finger is on the senosr and it loges the chnaegs, it also automagically reports every 60 seconds or when the finger is removed, and it also reads and processes the bluetooth commands
void loop() {
    int signalAmplitude = pulseSensor.getLatestSample();
    bool currentFingerDetected = checkFingerPlacement(signalAmplitude);

    if (currentFingerDetected != isFingerDetected) {
        isFingerDetected = currentFingerDetected;

        if (isFingerDetected) {
            LOG_INFO("Finger detected. Starting heart rate tracking.");
            currentSession.sessionStartTime = millis();
        } else {
            LOG_INFO("Finger removed. Reporting session statistics.");
            reportSessionStatistics();
        }
    }

    if (isFingerDetected) {
        int bpm = pulseSensor.getBeatsPerMinute();

        if (pulseSensor.sawStartOfBeat() && bpm >= 30 && bpm <= 200) {
            updateHeartRate(bpm);
        }

        unsigned long currentTime = millis();
        if (currentTime - currentSession.sessionStartTime >= REPORT_INTERVAL_MS) {
            reportSessionStatistics();
        }
    }

    while (Bluetooth.available()) {
        char receivedChar = Bluetooth.read();
        if (receivedChar == '\n' || bluetoothBufferIndex >= BLUETOOTH_BUFFER_SIZE - 1) {
            bluetoothBuffer[bluetoothBufferIndex] = '\0';
            processBluetoothCommand(bluetoothBuffer);
            bluetoothBufferIndex = 0;
        } else {
            bluetoothBuffer[bluetoothBufferIndex++] = receivedChar;
        }
    }
}

// okay this initlialises the serial and the bluetooth interfcaes, so basically checks if the pulse sensor is working or not if its not then it logs an error
void initializeSystem() {
    Serial.begin(9600);
    Bluetooth.begin(9600);

    LOG_DEBUG("Setup started.");

    pulseSensor.analogInput(PULSE_SENSOR_PIN);
    pulseSensor.blinkOnPulse(HEARTBEAT_LED_PIN);
    pulseSensor.setThreshold(DEFAULT_SIGNAL_THRESHOLD);

    if (!pulseSensor.begin()) {
        LOG_ERROR("Pulse Sensor initialization failed.");
        exit(EXIT_FAILURE);
    }

    LOG_INFO("Pulse Sensor initialized successfully.");
}

// this si the message that logs to both the serial monitor and the bluetooth terminal on the phone
void logMessage(const char *level, const char *message) {
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "[%lu ms] [%s] %s", millis(), level, message);
    Serial.println(buffer);
    Bluetooth.println(buffer);
}

//
void processBluetoothCommand(const char *command) {
    LOG_DEBUG("Processing Bluetooth command.");
    if (strncmp(command, "SET_THRESHOLD:", 14) == 0) {
        int threshold = atoi(command + 14);
        pulseSensor.setThreshold(threshold);
        LOG_INFO("Threshold updated.");
    } else {
        LOG_ERROR("Unknown command received.");
    }
}


void updateHeartRate(int bpm) {
    currentSession.totalHeartbeats++;
    currentSession.totalBPM += bpm;  
    currentSession.bpmReadingsCount++;  

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "BPM: %d, Total Heartbeats: %d", bpm, currentSession.totalHeartbeats);
    LOG_DATA(buffer);
}


void sendRecommendation(float averageBPM) {
    if (averageBPM > DEFAULT_MAX_HEART_RATE) {
        LOG_INFO("Recommendation: Relax, practice deep breathing.");
    } else if (averageBPM < DEFAULT_MIN_HEART_RATE) {
        LOG_INFO("Recommendation: Light activity to increase heart rate.");
    } else {
        LOG_INFO("Heart rate is good. Maintain a healthy lifestyle.");
    }
}

bool checkFingerPlacement(int signalAmplitude) {
    return signalAmplitude > NO_FINGER_SIGNAL_THRESHOLD;
}


void reportSessionStatistics() {
 
    float averageBPM = calculateAverageBPM();

    if (currentSession.bpmReadingsCount > 0) { 
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "Average BPM: %.2f, Total Heartbeats: %d",
                 averageBPM, currentSession.totalHeartbeats);
        LOG_REPORT(buffer);  
        sendRecommendation(averageBPM);  
    } else {
        LOG_WARNING("No valid heart rate readings to report.");
    }


    currentSession.totalHeartbeats = 0;
    currentSession.totalBPM = 0;
    currentSession.bpmReadingsCount = 0;
    currentSession.sessionStartTime = millis();
}
