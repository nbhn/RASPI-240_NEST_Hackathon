import os
import cv2
import face_recognition
import pickle
import numpy as np
import time
from datetime import datetime

class FaceRecognitionSystem:
    def __init__(self):
        self.known_face_encodings = []
        self.known_face_names = []
        self.model_file = "face_recognition_model.pkl"
        self.recognition_threshold = 0.6  # Adjustable threshold (lower = stricter matching)
        
    def load_model(self):
        """Load the face recognition model if it exists"""
        if os.path.exists(self.model_file):
            with open(self.model_file, 'rb') as f:
                data = pickle.load(f)
                self.known_face_encodings = data["encodings"]
                self.known_face_names = data["names"]
            print(f"Model loaded with {len(self.known_face_names)} faces")
            print(f"Recognized people: {', '.join(set(self.known_face_names))}")
            return True
        return False
            
    def save_model(self):
        """Save the face recognition model"""
        data = {"encodings": self.known_face_encodings, "names": self.known_face_names}
        with open(self.model_file, 'wb') as f:
            pickle.dump(data, f)
        print(f"Model saved with {len(self.known_face_names)} faces")
    
    def train_face(self, name):
        """Capture and train on a person's face"""
        if not os.path.exists("training_images"):
            os.makedirs("training_images")
            
        person_dir = f"training_images/{name}"
        if not os.path.exists(person_dir):
            os.makedirs(person_dir)
            
        print(f"\nTraining mode for: {name}")
        print("Looking at the camera, I'll capture 5 images of your face.")
        print("Press SPACE to capture each image, ESC to cancel")
        
        cap = cv2.VideoCapture(0)
        if not cap.isOpened():
            print("Error: Could not open webcam")
            return False
            
        img_count = 0
        while img_count < 5:
            ret, frame = cap.read()
            if not ret:
                print("Failed to grab frame")
                break
                
            # Mirror the image horizontally for more intuitive preview
            frame = cv2.flip(frame, 1)
            
            # Display counter
            cv2.putText(frame, f"Capture {img_count+1}/5", (20, 40), 
                        cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
            
            # Display instructions
            cv2.putText(frame, "SPACE: Capture | ESC: Cancel", (20, frame.shape[0] - 20), 
                        cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 1)
                        
            # Find faces in current frame
            rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            face_locations = face_recognition.face_locations(rgb_frame)
            
            # Draw rectangles around detected faces
            for (top, right, bottom, left) in face_locations:
                cv2.rectangle(frame, (left, top), (right, bottom), (0, 255, 0), 2)
            
            cv2.imshow('Capture Training Images', frame)
            
            key = cv2.waitKey(1)
            if key == 27:  # ESC key
                print("Training cancelled")
                break
            elif key == 32:  # SPACE key
                if len(face_locations) == 0:
                    print("No face detected! Please try again.")
                    continue
                elif len(face_locations) > 1:
                    print("Multiple faces detected! Please ensure only one face is in frame.")
                    continue
                    
                # Save the image
                timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                img_path = f"{person_dir}/{name}_{timestamp}_{img_count}.jpg"
                cv2.imwrite(img_path, frame)
                print(f"Image {img_count+1} captured and saved to {img_path}!")
                
                # Process the image for face encoding
                try:
                    # Convert to RGB explicitly before encoding
                    rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                    face_encoding = face_recognition.face_encodings(rgb_frame, face_locations)[0]
                    self.known_face_encodings.append(face_encoding)
                    self.known_face_names.append(name)
                    img_count += 1
                    time.sleep(1)  # Small delay to prepare for next pose
                except Exception as e:
                    print(f"Error processing face: {e}")
                    print("Please try again with a clearer view of your face.")
        
        cap.release()
        cv2.destroyAllWindows()
        
        if img_count > 0:
            self.save_model()
            return True
        return False
    
    def start_recognition(self):
        """Start real-time face recognition using webcam"""
        # Load existing model if available
        self.load_model()
        
        if len(self.known_face_encodings) == 0:
            print("No faces trained yet! Please train at least one face first.")
            return
            
        print("\nStarting face recognition...")
        print(f"Recognition threshold: {self.recognition_threshold} (lower = stricter matching)")
        print("Press 't' to adjust threshold, 'q' to quit")
        
        cap = cv2.VideoCapture(0)
        if not cap.isOpened():
            print("Error: Could not open webcam")
            return
            
        process_every_n_frames = 2
        frame_count = 0
        
        while True:
            ret, frame = cap.read()
            if not ret:
                print("Failed to grab frame")
                break
                
            # Mirror the image horizontally for more intuitive display
            frame = cv2.flip(frame, 1)
            
            # Show current threshold on screen
            cv2.putText(frame, f"Threshold: {self.recognition_threshold:.2f}", 
                      (10, frame.shape[0] - 20), cv2.FONT_HERSHEY_SIMPLEX, 
                      0.6, (255, 255, 255), 1)
            
            # Process only every nth frame to improve performance
            if frame_count % process_every_n_frames == 0:
                # Resize frame for faster processing
                small_frame = cv2.resize(frame, (0, 0), fx=0.25, fy=0.25)
                
                # Convert from BGR to RGB explicitly
                rgb_small_frame = cv2.cvtColor(small_frame, cv2.COLOR_BGR2RGB)
                
                # Find faces in current frame
                face_locations = face_recognition.face_locations(rgb_small_frame)
                
                # FIXED: Use face_landmarks instead of direct encoding to avoid the type error
                face_landmarks_list = []
                for face_location in face_locations:
                    landmarks = face_recognition.face_landmarks(rgb_small_frame, [face_location])
                    if landmarks:  # Make sure landmarks were detected
                        face_landmarks_list.extend(landmarks)
                
                # Now get encodings from the landmarks
                face_encodings = []
                for i, face_location in enumerate(face_locations):
                    try:
                        # Get face encodings directly from image
                        encoding = face_recognition.face_encodings(rgb_small_frame, [face_location])
                        if encoding:
                            face_encodings.append(encoding[0])
                    except Exception as e:
                        print(f"Error encoding face {i}: {e}")
                
                face_names = []
                face_confidence = []
                
                for face_encoding in face_encodings:
                    # Compare face with known faces
                    name = "Unknown"
                    confidence = 0
                    
                    if len(self.known_face_encodings) > 0:
                        # Calculate face distances
                        face_distances = face_recognition.face_distance(self.known_face_encodings, face_encoding)
                        
                        if len(face_distances) > 0:
                            # Get best match
                            best_match_index = np.argmin(face_distances)
                            best_match_distance = face_distances[best_match_index]
                            
                            # Convert distance to a similarity score (0-1, higher is better match)
                            # face_distance returns values where lower means more similar
                            similarity = 1 - best_match_distance
                            confidence = similarity
                            
                            # Apply threshold for recognition
                            if similarity > (1 - self.recognition_threshold):
                                name = self.known_face_names[best_match_index]
                    
                    face_names.append(name)
                    face_confidence.append(confidence)
                    
                # Display results
                for (top, right, bottom, left), name, confidence in zip(face_locations, face_names, face_confidence):
                    # Scale back face locations since we processed a resized frame
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
            
            cv2.imshow('Face Recognition', frame)
            
            frame_count += 1
            
            key = cv2.waitKey(1) & 0xFF
            if key == ord('q'):
                break
            elif key == ord('t'):
                # Allow user to adjust threshold
                try:
                    new_threshold = float(input("\nEnter new recognition threshold (0.1-0.9, lower = stricter): "))
                    if 0.1 <= new_threshold <= 0.9:
                        self.recognition_threshold = new_threshold
                        print(f"Threshold updated to {self.recognition_threshold}")
                    else:
                        print("Threshold must be between 0.1 and 0.9")
                except ValueError:
                    print("Invalid input, threshold not changed")
                
        cap.release()
        cv2.destroyAllWindows()
        
    def adjust_settings(self):
        """Adjust recognition settings"""
        print("\n===== Recognition Settings =====")
        print(f"Current recognition threshold: {self.recognition_threshold}")
        print("(Lower threshold = stricter matching, higher = more lenient)")
        
        try:
            new_threshold = float(input("Enter new threshold (0.1-0.9): "))
            if 0.1 <= new_threshold <= 0.9:
                self.recognition_threshold = new_threshold
                print(f"Threshold updated to {self.recognition_threshold}")
            else:
                print("Threshold must be between 0.1 and 0.9")
        except ValueError:
            print("Invalid input, threshold not changed")
        
def main():
    face_system = FaceRecognitionSystem()
    
    while True:
        print("\n===== Face Recognition System =====")
        print("1. Train a new face")
        print("2. Start face recognition")
        print("3. Adjust recognition settings")
        print("4. Clear all trained faces")
        print("5. Exit")
        
        choice = input("\nEnter your choice (1-5): ")
        
        if choice == '1':
            name = input("Enter the person's name: ")
            face_system.train_face(name)
        elif choice == '2':
            face_system.start_recognition()
        elif choice == '3':
            face_system.adjust_settings()
        elif choice == '4':
            confirm = input("Are you sure you want to clear all trained faces? (y/n): ")
            if confirm.lower() == 'y':
                if os.path.exists(face_system.model_file):
                    os.remove(face_system.model_file)
                    face_system.known_face_encodings = []
                    face_system.known_face_names = []
                    print("All trained faces have been cleared.")
                else:
                    print("No trained faces to clear.")
        elif choice == '5':
            print("Exiting...")
            break
        else:
            print("Invalid choice. Please try again.")

if __name__ == "__main__":
    main()