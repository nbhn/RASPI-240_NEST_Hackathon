import os
import sys
import cv2
import face_recognition
import pickle
import numpy as np
import time
from datetime import datetime
from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, 
                            QPushButton, QLabel, QComboBox, QListWidget, QListWidgetItem,
                            QInputDialog, QMessageBox, QSlider, QGroupBox, QSplitter,
                            QFrame, QLineEdit, QProgressBar, QTableWidget, QTableWidgetItem,
                            QHeaderView)
from PyQt5.QtGui import QImage, QPixmap, QFont, QIcon, QColor
from PyQt5.QtCore import Qt, QTimer, pyqtSignal, QThread, QDateTime, QSize

class VideoThread(QThread):
    change_pixmap_signal = pyqtSignal(np.ndarray)
    recognized_faces_signal = pyqtSignal(list)
    stats_signal = pyqtSignal(dict)
    
    def __init__(self, face_system, parent=None):
        super().__init__(parent)
        self.face_system = face_system
        self.running = True
        self.mode = "recognition"  # "recognition" or "training"
        self.training_name = ""
        self.capture_next = False
        self.process_every_n_frames = 2
        self.current_faces = []
        
    def run(self):
        cap = cv2.VideoCapture(0)
        if not cap.isOpened():
            QMessageBox.critical(None, "Error", "Could not open webcam")
            return
            
        frame_count = 0
        training_count = 0
        self.face_system.load_model()
        
        while self.running:
            ret, frame = cap.read()
            if not ret:
                break
                
            # Mirror the image horizontally
            frame = cv2.flip(frame, 1)
            
            if self.mode == "training":
                # Add training overlay
                cv2.putText(frame, f"Training: {self.training_name}", (20, 40), 
                            cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
                cv2.putText(frame, f"Captured: {training_count}/5", (20, 80), 
                            cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
                
                # Find and highlight faces
                rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                face_locations = face_recognition.face_locations(rgb_frame)
                
                for (top, right, bottom, left) in face_locations:
                    cv2.rectangle(frame, (left, top), (right, bottom), (0, 255, 0), 2)
                
                # Capture image when requested
                if self.capture_next and len(face_locations) == 1:
                    if not os.path.exists("training_images"):
                        os.makedirs("training_images")
                    
                    person_dir = f"training_images/{self.training_name}"
                    if not os.path.exists(person_dir):
                        os.makedirs(person_dir)
                    
                    # Save the image
                    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                    img_path = f"{person_dir}/{self.training_name}_{timestamp}_{training_count}.jpg"
                    cv2.imwrite(img_path, frame)
                    
                    # Process for face encoding
                    try:
                        face_encoding = face_recognition.face_encodings(rgb_frame, face_locations)[0]
                        self.face_system.known_face_encodings.append(face_encoding)
                        self.face_system.known_face_names.append(self.training_name)
                        training_count += 1
                        self.face_system.save_model()
                    except Exception as e:
                        print(f"Error processing face: {e}")
                    
                    self.capture_next = False
                    
                    # Check if training complete
                    if training_count >= 5:
                        self.mode = "recognition"
                        QMessageBox.information(None, "Training Complete", 
                                            f"Training complete for {self.training_name}")
                
            elif self.mode == "recognition" and frame_count % self.process_every_n_frames == 0:
                # Process for recognition
                small_frame = cv2.resize(frame, (0, 0), fx=0.25, fy=0.25)
                rgb_small_frame = cv2.cvtColor(small_frame, cv2.COLOR_BGR2RGB)
                
                # Find faces
                face_locations = face_recognition.face_locations(rgb_small_frame)
                face_encodings = []
                
                for face_location in face_locations:
                    try:
                        encodings = face_recognition.face_encodings(rgb_small_frame, [face_location])
                        if encodings:
                            face_encodings.append(encodings[0])
                    except Exception as e:
                        print(f"Error encoding face: {e}")
                
                face_names = []
                face_confidence = []
                detected_faces = []
                
                for face_encoding in face_encodings:
                    name = "Unknown"
                    confidence = 0
                    
                    if len(self.face_system.known_face_encodings) > 0:
                        face_distances = face_recognition.face_distance(
                            self.face_system.known_face_encodings, face_encoding)
                        
                        if len(face_distances) > 0:
                            best_match_index = np.argmin(face_distances)
                            best_match_distance = face_distances[best_match_index]
                            
                            similarity = 1 - best_match_distance
                            confidence = similarity
                            
                            if similarity > (1 - self.face_system.recognition_threshold):
                                name = self.face_system.known_face_names[best_match_index]
                    
                    face_names.append(name)
                    face_confidence.append(confidence)
                    
                    # Record detection with timestamp
                    if name != "Unknown":
                        detected_faces.append({
                            'name': name,
                            'confidence': confidence,
                            'timestamp': datetime.now().strftime("%H:%M:%S")
                        })
                
                # Update recognitions list via signal
                self.recognized_faces_signal.emit(detected_faces)
                    
                # Calculate stats for people seen
                stats = {}
                for name, conf in zip(face_names, face_confidence):
                    if name != "Unknown":
                        if name not in stats:
                            stats[name] = {'count': 1, 'avg_conf': conf}
                        else:
                            stats[name]['count'] += 1
                            # Update running average
                            stats[name]['avg_conf'] = (stats[name]['avg_conf'] * 
                                                      (stats[name]['count'] - 1) + conf) / stats[name]['count']
                
                if stats:
                    self.stats_signal.emit(stats)
                
                # Draw boxes and labels for faces
                for (top, right, bottom, left), name, confidence in zip(face_locations, face_names, face_confidence):
                    # Scale back face locations
                    top *= 4
                    right *= 4
                    bottom *= 4
                    left *= 4
                    
                    # Draw rectangle around the face
                    if name == "Unknown":
                        color = (0, 0, 255)  # Red for unknown
                    else:
                        color = (0, 255, 0)  # Green for known
                        
                    cv2.rectangle(frame, (left, top), (right, bottom), color, 2)
                    
                    # Draw name label with confidence
                    conf_text = f"{confidence:.2f}" if name != "Unknown" else ""
                    label = f"{name} {conf_text}"
                    cv2.rectangle(frame, (left, bottom - 35), (right, bottom), color, cv2.FILLED)
                    cv2.putText(frame, label, (left + 6, bottom - 6), 
                                cv2.FONT_HERSHEY_DUPLEX, 0.8, (255, 255, 255), 1)
                
                # Store current faces for the UI
                self.current_faces = list(zip(face_names, face_confidence))
            
            # Add threshold information
            cv2.putText(frame, f"Threshold: {self.face_system.recognition_threshold:.2f}", 
                      (10, frame.shape[0] - 20), cv2.FONT_HERSHEY_SIMPLEX, 
                      0.6, (255, 255, 255), 1)
            
            frame_count += 1
            
            # Emit the frame for display
            self.change_pixmap_signal.emit(frame)
            
            # Control the update rate
            time.sleep(0.03)
            
        cap.release()
        
    def capture_training_image(self):
        self.capture_next = True
        
    def stop(self):
        self.running = False
        self.wait()

class FaceRecognitionSystem:
    def __init__(self):
        self.known_face_encodings = []
        self.known_face_names = []
        self.model_file = "face_recognition_model.pkl"
        self.recognition_threshold = 1 - 0.7
        self.detection_history = []  # To store recognition history
        
    def load_model(self):
        """Load the face recognition model if it exists"""
        if os.path.exists(self.model_file):
            with open(self.model_file, 'rb') as f:
                data = pickle.load(f)
                self.known_face_encodings = data["encodings"]
                self.known_face_names = data["names"]
            print(f"Model loaded with {len(self.known_face_names)} faces")
            return True
        return False
            
    def save_model(self):
        """Save the face recognition model"""
        data = {"encodings": self.known_face_encodings, "names": self.known_face_names}
        with open(self.model_file, 'wb') as f:
            pickle.dump(data, f)
        print(f"Model saved with {len(self.known_face_names)} faces")
        
    def get_unique_people(self):
        """Get list of unique people in the model"""
        return sorted(set(self.known_face_names))

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        
        self.face_system = FaceRecognitionSystem()
        self.face_system.load_model()
        
        self.init_ui()
        
        # Start video thread
        self.video_thread = VideoThread(self.face_system)
        self.video_thread.change_pixmap_signal.connect(self.update_image)
        self.video_thread.recognized_faces_signal.connect(self.update_recognitions)
        self.video_thread.stats_signal.connect(self.update_stats)
        self.video_thread.start()
        
        # Start timer for updating time
        self.time_timer = QTimer(self)
        self.time_timer.timeout.connect(self.update_time)
        self.time_timer.start(1000)
        
        self.update_people_list()
        
    def init_ui(self):
        self.setWindowTitle('Face Recognition System')
        self.setGeometry(100, 100, 1200, 800)
        
        # Main layout
        main_layout = QHBoxLayout()
        
        # Left panel - controls and recognition info
        left_panel = QWidget()
        left_layout = QVBoxLayout(left_panel)
        
        # Header with current time
        self.time_label = QLabel()
        self.time_label.setAlignment(Qt.AlignCenter)
        self.time_label.setFont(QFont('Arial', 14))
        self.time_label.setStyleSheet("background-color: #333; color: white; padding: 8px;")
        left_layout.addWidget(self.time_label)
        
        # Controls group
        controls_group = QGroupBox("Controls")
        controls_layout = QVBoxLayout()
        
        # Train new face button
        self.train_btn = QPushButton("Train New Face")
        self.train_btn.clicked.connect(self.train_new_face)
        controls_layout.addWidget(self.train_btn)
        
        # Threshold slider
        threshold_layout = QHBoxLayout()
        threshold_layout.addWidget(QLabel("Recognition Threshold:"))
        self.threshold_slider = QSlider(Qt.Horizontal)
        self.threshold_slider.setMinimum(1)
        self.threshold_slider.setMaximum(9)
        self.threshold_slider.setValue(int(self.face_system.recognition_threshold * 10))
        self.threshold_slider.setTickPosition(QSlider.TicksBelow)
        self.threshold_slider.setTickInterval(1)
        self.threshold_slider.valueChanged.connect(self.threshold_changed)
        threshold_layout.addWidget(self.threshold_slider)
        self.threshold_value_label = QLabel(f"{self.face_system.recognition_threshold:.1f}")
        threshold_layout.addWidget(self.threshold_value_label)
        controls_layout.addLayout(threshold_layout)
        
        # Clear data button
        self.clear_btn = QPushButton("Clear All Face Data")
        self.clear_btn.clicked.connect(self.clear_face_data)
        controls_layout.addWidget(self.clear_btn)
        
        controls_group.setLayout(controls_layout)
        left_layout.addWidget(controls_group)
        
        # People in database group
        people_group = QGroupBox("People in Database")
        people_layout = QVBoxLayout()
        self.people_list = QListWidget()
        self.people_list.setAlternatingRowColors(True)
        people_layout.addWidget(self.people_list)
        people_group.setLayout(people_layout)
        left_layout.addWidget(people_group)
        
        # Recent recognitions group
        recognitions_group = QGroupBox("Recent Recognitions")
        recognitions_layout = QVBoxLayout()
        self.recognitions_table = QTableWidget(0, 3)
        self.recognitions_table.setHorizontalHeaderLabels(["Person", "Confidence", "Time"])
        self.recognitions_table.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        self.recognitions_table.setEditTriggers(QTableWidget.NoEditTriggers)
        self.recognitions_table.setAlternatingRowColors(True)
        recognitions_layout.addWidget(self.recognitions_table)
        recognitions_group.setLayout(recognitions_layout)
        left_layout.addWidget(recognitions_group)
        
        # Stats group
        stats_group = QGroupBox("Statistics")
        stats_layout = QVBoxLayout()
        self.stats_table = QTableWidget(0, 2)
        self.stats_table.setHorizontalHeaderLabels(["Person", "Confidence"])
        self.stats_table.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        self.stats_table.setEditTriggers(QTableWidget.NoEditTriggers)
        self.stats_table.setAlternatingRowColors(True)
        stats_layout.addWidget(self.stats_table)
        stats_group.setLayout(stats_layout)
        left_layout.addWidget(stats_group)
        
        # Right panel - video feed
        right_panel = QWidget()
        right_layout = QVBoxLayout(right_panel)
        
        # Video feed
        self.video_label = QLabel()
        self.video_label.setAlignment(Qt.AlignCenter)
        self.video_label.setMinimumSize(640, 480)
        self.video_label.setStyleSheet("border: 2px solid #ccc;")
        right_layout.addWidget(self.video_label)
        
        # Current detection info
        self.detection_label = QLabel("No faces detected")
        self.detection_label.setAlignment(Qt.AlignCenter)
        self.detection_label.setFont(QFont('Arial', 12))
        self.detection_label.setStyleSheet("background-color: #000000; padding: 10px;")
        right_layout.addWidget(self.detection_label)
        
        # Training controls (initially hidden)
        self.training_widget = QWidget()
        training_layout = QHBoxLayout(self.training_widget)
        
        self.capture_btn = QPushButton("Capture Image")
        self.capture_btn.clicked.connect(self.capture_training_image)
        training_layout.addWidget(self.capture_btn)
        
        self.cancel_training_btn = QPushButton("Cancel Training")
        self.cancel_training_btn.clicked.connect(self.cancel_training)
        training_layout.addWidget(self.cancel_training_btn)
        
        self.training_widget.setVisible(False)
        right_layout.addWidget(self.training_widget)
        
        # Add panels to splitter
        splitter = QSplitter(Qt.Horizontal)
        splitter.addWidget(left_panel)
        splitter.addWidget(right_panel)
        splitter.setSizes([400, 800])
        
        # Set splitter as central widget
        central_widget = QWidget()
        main_layout.addWidget(splitter)
        central_widget.setLayout(main_layout)
        self.setCentralWidget(central_widget)
        
        # Set application style
        self.setStyleSheet("""
            QMainWindow {
                background-color: #000000; /* pure white background */
            }

            QGroupBox {
                font-weight: bold;
                border: 1px solid #999999; /* darker border for more contrast */
                border-radius: 5px;
                margin-top: 10px;
                padding-top: 10px;
            }

            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 5px;
            }

            QPushButton {
                background-color: #0052cc; /* deeper blue */
                color: white;
                border: none;
                padding: 8px;
                border-radius: 4px;
            }

            QPushButton:hover {
                background-color: #003d99;
            }

            QPushButton:pressed {
                background-color: #002966;
            }

            QTableWidget {
                gridline-color: #cccccc;
                selection-background-color: #000000; 
                selection-color: white; 
            }

            QHeaderView::section {
                background-color: #000000; 
                padding: 4px;
                border: 1px solid #bbbbbb;
                font-weight: bold;
            }
            """)
        
    def update_image(self, cv_img):
        """Updates the image_label with a new opencv image"""
        qt_img = self.convert_cv_qt(cv_img)
        self.video_label.setPixmap(qt_img)
        
    def convert_cv_qt(self, cv_img):
        """Convert from an opencv image to QPixmap"""
        rgb_image = cv2.cvtColor(cv_img, cv2.COLOR_BGR2RGB)
        h, w, ch = rgb_image.shape
        bytes_per_line = ch * w
        convert_to_Qt_format = QImage(rgb_image.data, w, h, bytes_per_line, QImage.Format_RGB888)
        p = convert_to_Qt_format.scaled(self.video_label.width(), self.video_label.height(), Qt.KeepAspectRatio)
        return QPixmap.fromImage(p)
    
    def update_recognitions(self, faces):
        """Update the recognitions table with newly detected faces"""
        if not faces:
            return
            
        # Update detection label
        if faces:
            face_text = ", ".join([f"{face['name']} ({face['confidence']:.2f})" for face in faces])
            self.detection_label.setText(f"Detected: {face_text}")
        else:
            self.detection_label.setText("No faces detected")
            
        # Add new recognitions to the table (only known faces)
        known_faces = [face for face in faces if face['name'] != "Unknown"]
        if not known_faces:
            return
            
        for face in known_faces:
            # Add to history list (limit size)
            self.face_system.detection_history.append(face)
            if len(self.face_system.detection_history) > 100:
                self.face_system.detection_history.pop(0)
            
            # Add row to table
            row_position = self.recognitions_table.rowCount()
            self.recognitions_table.insertRow(row_position)
            
            # Set values
            self.recognitions_table.setItem(row_position, 0, QTableWidgetItem(face['name']))
            self.recognitions_table.setItem(row_position, 1, QTableWidgetItem(f"{face['confidence']:.2f}"))
            self.recognitions_table.setItem(row_position, 2, QTableWidgetItem(face['timestamp']))
            
            # Scroll to the new item
            self.recognitions_table.scrollToItem(self.recognitions_table.item(row_position, 0))
    
    def update_stats(self, stats):
        """Update the statistics table"""
        self.stats_table.setRowCount(0)
        
        for name, data in stats.items():
            row_position = self.stats_table.rowCount()
            self.stats_table.insertRow(row_position)
            
            self.stats_table.setItem(row_position, 0, QTableWidgetItem(name))
            self.stats_table.setItem(row_position, 1, QTableWidgetItem(f"{data['avg_conf']:.2f}"))
    
    def update_time(self):
        """Update the time display"""
        current_time = QDateTime.currentDateTime()
        time_display = current_time.toString('dddd, MMMM d, yyyy - hh:mm:ss AP')
        self.time_label.setText(time_display)
    
    def update_people_list(self):
        """Update the list of people in the database"""
        self.people_list.clear()
        unique_people = self.face_system.get_unique_people()
        
        for person in unique_people:
            # Count instances of this person in the model
            count = self.face_system.known_face_names.count(person)
            item = QListWidgetItem(f"{person} ({count} images)")
            self.people_list.addItem(item)
    
    def train_new_face(self):
        """Start training mode for a new face"""
        name, ok = QInputDialog.getText(self, "Train New Face", "Enter person's name:")
        
        if ok and name:
            # Switch to training mode
            self.video_thread.mode = "training"
            self.video_thread.training_name = name
            self.training_widget.setVisible(True)
            self.train_btn.setEnabled(False)
    
    def capture_training_image(self):
        """Capture an image during training"""
        self.video_thread.capture_training_image()
    
    def cancel_training(self):
        """Cancel the training process"""
        self.video_thread.mode = "recognition"
        self.training_widget.setVisible(False)
        self.train_btn.setEnabled(True)
        self.update_people_list()
    
    def threshold_changed(self):
        """Update threshold when slider is moved"""
        value = self.threshold_slider.value() / 10.0
        self.face_system.recognition_threshold = value
        self.threshold_value_label.setText(f"{value:.1f}")
    
    def clear_face_data(self):
        """Clear all face recognition data"""
        reply = QMessageBox.question(self, 'Confirmation',
                                    'Are you sure you want to clear all face data?',
                                    QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
        
        if reply == QMessageBox.Yes:
            if os.path.exists(self.face_system.model_file):
                os.remove(self.face_system.model_file)
                self.face_system.known_face_encodings = []
                self.face_system.known_face_names = []
                self.update_people_list()
                QMessageBox.information(self, "Cleared", "All face data has been cleared")
    
    def closeEvent(self, event):
        """Clean up when window is closed"""
        self.video_thread.stop()
        event.accept()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())