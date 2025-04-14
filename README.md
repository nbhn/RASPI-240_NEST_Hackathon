# RASPI-240: Smart Industrial Monitoring System
## NEST Hackathon 2025 Project

![Project Banner](./media/banner.png)

## 🏆 NEST Hackathon Experience

Our team had an amazing experience at the NEST Hackathon 2025, where we developed RASPI-240: a comprehensive industrial monitoring and security system. This solution combines IoT sensors, machine learning, and web technologies to create a holistic approach to industrial safety and efficiency.

## 📋 Project Overview

RASPI-240 is an integrated system that provides:

- **Access Control**: RFID badge-based security for restricted areas
- **Temperature Monitoring**: Real-time temperature tracking with alerts
- **Facial Recognition**: AI-powered authentication for administrators
- **Failure Prediction**: Machine learning algorithms for preventive maintenance
- **Dashboard**: Web interface for monitoring and managing all systems

## 🎥 Demo Videos

### 3D System Architecture Visualization
Our system's physical layout and component interconnections in 3D:

[![3D System Plan](https://img.youtube.com/vi/RSE16WzwnU0/0.jpg)](https://youtu.be/RSE16WzwnU0)

### Facial Recognition Security System
AI-powered facial recognition for administrator access:

[![Face Recognition Demo](https://img.youtube.com/vi/oYCKodpSrzM/0.jpg)](https://youtu.be/oYCKodpSrzM)

### Interactive Dashboard & Site Operations
Complete walkthrough of our web dashboard and monitoring system:

[![Web Dashboard & Site Demo](https://img.youtube.com/vi/Pgzu2_fCa18/0.jpg)](https://youtu.be/Pgzu2_fCa18)

## 🧩 System Components

Our solution consists of several interconnected components:

### 1. RFID Access Control
Arduino-based system for controlling access to restricted areas using RFID badges. [Details](./docs/rfid_access.md)

### 2. Temperature Control
Multi-sensor temperature monitoring system with alerting capabilities. [Details](./docs/temperature_control.md)

### 3. MQTT Bridge
Node.js service that connects hardware components to our Supabase database using MQTT protocol. [Details](./docs/mqtt_bridge.md)

### 4. Face Recognition System
AI-powered facial recognition system for administrator authentication. [Details](./docs/face_recognition.md)

### 5. Failure Prediction System
Machine learning models for predicting equipment failures before they occur. [Details](./docs/failure_prediction.md)

### 6. Web Dashboard
React-based web interface that provides real-time monitoring and control of all system components. [Details](./docs/web_dashboard.md)

## 🔌 Hardware Components

Our system utilizes the following hardware components:

### Computing & Control
- **Development PC**: Intel Core i7, 32GB RAM for development and testing
- **Arduino Uno R3**: Main microcontroller for RFID access control
- **Arduino Nano**: Secondary controller for remote temperature sensors
- **USB Cables**: For connecting Arduino boards to development PC

### Access Control
- **RFID-RC522 Module**: 13.56MHz RFID reader/writer module
- **RFID Tags/Cards**: Mifare Classic 1K cards for user authentication
- **LED Indicators**: Red/Green LEDs for access status indication
- **Buzzer Module**: For audio feedback during access attempts
- **Relay Module**: For electric door lock control

### Temperature Monitoring
- **DS18B20 Temperature Sensors**: Waterproof digital temperature sensors
- **DHT22 Sensors**: Temperature and humidity sensors for ambient monitoring
- **MAX6675 Module**: K-type thermocouple for high-temperature monitoring
- **LCD Display**: 16x2 I2C LCD for local temperature readings

### Networking & Connectivity
- **ESP8266 Wi-Fi Module**: For wireless connectivity
- **Ethernet Shield**: For wired network connectivity
- **Jumper Wires**: For connecting components
- **Breadboards**: For prototyping circuits

### Power & Enclosure
- **Power Adapters**: 5V and 12V power supplies
- **Project Box Enclosures**: For housing electronics
- **Battery Backup**: 9V battery with voltage regulator for failsafe operation

## 🚀 Installation & Setup

Please refer to our [Installation Guide](./docs/installation.md) for detailed setup instructions.

## 🔧 Technologies Used

- **Hardware**: PC, Arduino, RFID Readers, Temperature Sensors
- **Backend**: Node.js, MQTT (HiveMQ), Supabase
- **Frontend**: React, Vite, TailwindCSS
- **ML/AI**: TensorFlow, OpenCV, Custom ML Models
- **DevOps**: GitHub Actions, Vercel

## 👥 Team

- Nassim Benhalldja
- Arabet Abdelhakim
- Derar Mohanned
- Baslimane Daoud Elhakim
- HAMAIDI Mohamed Idris Hamadi

## 📚 Resources

- [HiveMQ Documentation](https://www.hivemq.com/docs/)
- [RandomNerdTutorials RFID Guide](https://randomnerdtutorials.com/)
- [Supabase Documentation](https://supabase.io/docs)

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

Developed with ❤️ for the NEST Hackathon 2025
