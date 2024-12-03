//this code is an exercise its not completely same but it set the ground//

import numpy as np
import cv2
import sys
from cvzone.FaceDetectionModule import FaceDetector
from cvzone.PlotModule import LivePlot
import cvzone
import time
from dataclasses import dataclass
import logging

@dataclass
class WebcamConfig:
    real_width: int = 640
    real_height: int = 480
    video_width: int = 160
    video_height: int = 120
    video_channels: int = 3
    video_frame_rate: int = 15


@dataclass
class ColorMagnificationConfig:
    levels: int = 3
    alpha: int = 170
    min_frequency: float = 1.0
    max_frequency: float = 2.0
    buffer_size: int = 150


@dataclass
class TextDisplayConfig:
    font: int = cv2.FONT_HERSHEY_SIMPLEX
    loading_text_location: tuple = (30, 40)
    bpm_text_location: tuple = (80, 40)
    fps_text_location: tuple = (30, 440)
    font_scale: float = 1
    font_color: tuple = (255, 255, 255)
    line_type: int = 2
    box_color: tuple = (0, 255, 0)
    box_weight: int = 3


class HeartRateMonitor:
    def __init__(self, webcam_config: WebcamConfig, magnification_config: ColorMagnificationConfig):
        self.webcam_config = webcam_config
        self.magnification_config = magnification_config
        self.webcam = cv2.VideoCapture(0)
        self.detector = FaceDetector()
        self.plot = LivePlot(webcam_config.real_width, webcam_config.real_height, [60, 120], invert=True)
        self.init_camera()
        self.buffer_index = 0
        self.bpm_buffer_index = 0
        self.bpm_buffer = np.zeros(10)
        self.video_gauss = np.zeros((magnification_config.buffer_size, webcam_config.video_width, webcam_config.video_height, webcam_config.video_channels))
        self.frequencies = self.init_frequencies()
        self.mask = (self.frequencies >= magnification_config.min_frequency) & (self.frequencies <= magnification_config.max_frequency)

    def init_camera(self):
        """Initializes camera settings."""
        self.webcam.set(3, self.webcam_config.real_width)
        self.webcam.set(4, self.webcam_config.real_height)

    def init_frequencies(self) -> np.ndarray:
        """Initializes frequency mask for filtering."""
        return (1.0 * self.webcam_config.video_frame_rate) * np.arange(self.magnification_config.buffer_size) / (1.0 * self.magnification_config.buffer_size)

    def build_gauss(self, frame: np.ndarray, levels: int) -> list:
        """Builds a Gaussian pyramid."""
        pyramid = [frame]
        for _ in range(levels):
            frame = cv2.pyrDown(frame)
            pyramid.append(frame)
        return pyramid

    def reconstruct_frame(self, pyramid: list, index: int, levels: int) -> np.ndarray:
        """Reconstructs the frame from the Gaussian pyramid."""
        filtered_frame = pyramid[index]
        for _ in range(levels):
            filtered_frame = cv2.pyrUp(filtered_frame)
        return filtered_frame[:self.webcam_config.video_height, :self.webcam_config.video_width]

    def process_frame(self, frame: np.ndarray):
        """Processes a single frame for heart rate monitoring."""
        try:
            frame, bboxs = self.detector.findFaces(frame, draw=False)
            if not bboxs:
                return frame

            x1, y1, w1, h1 = bboxs[0]['bbox']
            detection_frame = frame[y1:y1 + h1, x1:x1 + w1]
            detection_frame = cv2.resize(detection_frame, (self.webcam_config.video_width, self.webcam_config.video_height))
            self.video_gauss[self.buffer_index] = self.build_gauss(detection_frame, self.magnification_config.levels + 1)[-1]

            fourier_transform = np.fft.fft(self.video_gauss, axis=0)
            fourier_transform[self.mask == False] = 0

            filtered = np.real(np.fft.ifft(fourier_transform, axis=0)) * self.magnification_config.alpha
            output_frame = detection_frame + self.reconstruct_frame(filtered, self.buffer_index, self.magnification_config.levels)
            self.buffer_index = (self.buffer_index + 1) % self.magnification_config.buffer_size
            return cv2.convertScaleAbs(output_frame)
        except Exception as e:
            logging.error(f"Error processing frame: {e}")
            return frame

    def run(self):
        """Main loop for the heart rate monitor."""
        try:
            while True:
                ret, frame = self.webcam.read()
                if not ret:
                    logging.warning("No frame read from webcam. Exiting.")
                    break

                processed_frame = self.process_frame(frame)
                cv2.imshow("Heart Rate Monitor", processed_frame)

                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break
        finally:
            self.webcam.release()
            cv2.destroyAllWindows()


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO, format="%(asctime)s - %(levelname)s - %(message)s")
    webcam_config = WebcamConfig()
    magnification_config = ColorMagnificationConfig()
    monitor = HeartRateMonitor(webcam_config, magnification_config)
    monitor.run()
