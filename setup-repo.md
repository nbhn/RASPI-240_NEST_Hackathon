# Repository Setup Guide

This guide provides instructions for setting up the RASPI-240_NEST_Hackathon repository structure and pushing it to GitHub.

## Directory Structure

Create the following directory structure:

```
RASPI-240_NEST_Hackathon/
├── README.md
├── docs/
│   ├── rfid_access_control.md
│   ├── temperature_control.md
│   ├── mqtt_bridge.md
│   ├── face_recognition.md
│   ├── failure_prediction.md
│   └── web_dashboard.md
├── RFIDAccessControl/
│   ├── RFIDAccessControl.ino
│   └── README.md
├── TemperatureControl/
│   ├── TemperatureControl.ino
│   └── README.md
├── mqtt-bridge/
│   ├── rfid-bridge.js
│   ├── index.js
│   └── .env.example
├── face_recognition/
│   ├── face_reco.py
│   ├── face_reco_pyqt.py
│   ├── face_recognition_model.pkl
│   ├── requirements.txt
│   └── readme.md
├── failure_pred/
│   ├── degradation.ipynb
│   ├── degraradation2.ipynb
│   ├── full_server_degradation_dataset.csv
│   ├── server_degradation_sample_400.csv
│   └── models/
├── media/
│   ├── 3d_plan.mp4
│   ├── face_recognition.mov
│   └── site_recording.mov
└── LICENSE
```

## Step 1: Clone the Repository

```bash
git clone https://github.com/nbhn/RASPI-240_NEST_Hackathon.git
cd RASPI-240_NEST_Hackathon
```

## Step 2: Create Directory Structure

```bash
# Create directories
mkdir -p docs
mkdir -p RFIDAccessControl
mkdir -p TemperatureControl
mkdir -p mqtt-bridge
mkdir -p face_recognition
mkdir -p failure_pred/models
mkdir -p media
```

## Step 3: Copy Documentation Files

Copy the documentation files to their respective directories:

- Copy `README.md` to the root directory
- Copy all `docs/*.md` files to the `docs/` directory

## Step 4: Copy Source Code Files

### RFID Access Control
Copy the RFID Access Control files:
```bash
cp /path/to/RFIDAccessControl.ino RFIDAccessControl/
cp /path/to/rfid_readme.md RFIDAccessControl/README.md
```

### Temperature Control
Copy the Temperature Control files:
```bash
cp /path/to/TemperatureControl.ino TemperatureControl/
cp /path/to/temp_readme.md TemperatureControl/README.md
```

### MQTT Bridge
Copy the MQTT Bridge files:
```bash
cp /path/to/rfid-bridge.js mqtt-bridge/
cp /path/to/index.js mqtt-bridge/
# Create an example .env file (remove sensitive credentials)
cp /path/to/.env mqtt-bridge/.env.example
```

### Face Recognition
Copy the Face Recognition files:
```bash
cp /path/to/face_reco.py face_recognition/
cp /path/to/face_reco_pyqt.py face_recognition/
cp /path/to/face_recognition_model.pkl face_recognition/
cp /path/to/requirements.txt face_recognition/
cp /path/to/face_readme.md face_recognition/readme.md
```

### Failure Prediction
Copy the Failure Prediction files:
```bash
cp /path/to/degradation.ipynb failure_pred/
cp /path/to/degraradation2.ipynb failure_pred/
cp /path/to/full_server_degradation_dataset.csv failure_pred/
cp /path/to/server_degradation_sample_400.csv failure_pred/
cp /path/to/failure_progression.png failure_pred/
cp /path/to/overall_model_performance.png failure_pred/
cp /path/to/server_degradation_visualization.png failure_pred/
cp /path/to/prediction_performance.png failure_pred/
```

## Step 5: Copy Media Files

Copy the media files (if available):
```bash
cp /path/to/3d_plan.mp4 media/
cp /path/to/face_recognition.mov media/
cp /path/to/site_recording.mov media/
```

## Step 6: Create LICENSE File

Create an MIT License file:
```bash
cat > LICENSE << 'EOL'
MIT License

Copyright (c) 2025 Team RASPI-240

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
EOL
```

## Step 7: Commit and Push to GitHub

```bash
# Initialize Git repository (if not already done)
git init

# Add all files
git add .

# Commit the changes
git commit -m "Initial commit: RASPI-240 NEST Hackathon Project"

# Set up remote (if not already done)
git remote add origin https://github.com/nbhn/RASPI-240_NEST_Hackathon.git

# Push to GitHub
git push -u origin main
```

## Step 8: Verify Repository Structure

Visit https://github.com/nbhn/RASPI-240_NEST_Hackathon to verify that all files and directories have been successfully pushed to GitHub.

## Note on Large Files

If any of the media files (videos) or datasets are too large for GitHub, consider using Git LFS (Large File Storage):

```bash
# Install Git LFS
git lfs install

# Track large files
git lfs track "*.mp4"
git lfs track "*.mov"
git lfs track "*.csv"

# Add .gitattributes
git add .gitattributes

# Commit and push as normal
git add .
git commit -m "Add large files with Git LFS"
git push
```

Alternatively, you can consider hosting large files elsewhere and including links in the documentation. 