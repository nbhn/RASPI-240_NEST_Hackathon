# RFID Access Control System

## Overview

The RFID Access Control System is a comprehensive security solution designed to manage and monitor access to restricted areas. It combines RFID technology with ultrasonic sensors to prevent unauthorized entry and detect tailgating attempts.

## Features

- **Secure Authentication**: Validates RFID cards against a locally stored database
- **Anti-Tailgating**: Uses ultrasonic sensors to detect unauthorized entries
- **Remote Management**: Allows administrators to add/remove cards remotely
- **Real-time Logging**: Records all access attempts to a central database
- **MQTT Integration**: Communicates with the cloud using MQTT protocol
- **Visual & Audio Feedback**: Provides clear user feedback via LCD display, LEDs, and buzzers

## Hardware Components

- **ESP32 WROOM 32 Microcontroller**: Main control unit
- **MFRC522 RFID Reader**: Reads RFID card data
- **HC-SR04 Ultrasonic Sensor**: Detects movement for tailgating prevention
- **16x2 I2C LCD Display**: User interface and status display
- **LEDs & Buzzer**: Visual and audio feedback
- **Servo Motor**: Door lock control mechanism
- **Push Button**: Admin mode activation

## Pin Configuration

| Component | ESP32 Pin |
|-----------|-----------|
| RFID RST  | GPIO 4    |
| RFID SS   | GPIO 5    |
| RFID MOSI | GPIO 23   |
| RFID MISO | GPIO 19   |
| RFID SCK  | GPIO 18   |
| LCD SDA   | GPIO 21   |
| LCD SCL   | GPIO 22   |
| Ultrasonic Trigger | GPIO 15 |
| Ultrasonic Echo    | GPIO 13 |
| Servo      | GPIO 26 |
| Buzzer     | GPIO 25 |
| Green LED  | GPIO 32 |
| Red LED    | GPIO 33 |
| Admin Button | GPIO 27 |

## How It Works

### Normal Operation

1. The system continuously scans for RFID cards
2. When a card is presented:
   - The RFID reader captures the card's UID
   - The system checks if the card is authorized
   - For authorized cards:
     - The green LED lights up
     - A success tone plays
     - The door unlocks
     - The LCD displays "Access Granted"
     - The event is logged to the MQTT server
   - For unauthorized cards:
     - The red LED lights up
     - A warning tone plays
     - The door remains locked
     - The LCD displays "Access Denied"
     - The event is logged to the MQTT server

### Anti-Tailgating System

1. After granting access, the ultrasonic sensor monitors the doorway
2. The sensor measures the distance to detect person movement
3. If multiple distance changes are detected (indicating multiple people):
   - An alarm is triggered
   - The event is logged as a security breach
   - The dashboard displays a security alert

### Admin Mode

1. Admin mode is activated by pressing the admin button
2. The LCD displays "ADMIN MODE"
3. The first card scan selects a card to add/remove
4. The second scan of the same card toggles its authorization status:
   - If the card was authorized, it is removed
   - If the card was unauthorized, it is added
5. The system returns to normal mode after a successful operation or timeout

## Security Features

- **Local Database**: Cards are stored in local memory for offline operation
- **Encrypted MQTT**: Communications use TLS/SSL encryption
- **Entry Monitoring**: Detects and alerts on suspicious entry patterns
- **Remote Disabling**: Cards can be disabled remotely via MQTT

## Integration with Other Systems

The RFID Access Control System integrates with:

- **MQTT Bridge**: For database synchronization and remote management
- **Web Dashboard**: For monitoring and control
- **Face Recognition System**: For administrator authentication

## Troubleshooting

### Common Issues and Solutions

| Issue | Possible Cause | Solution |
|-------|---------------|----------|
| LCD not displaying | Wrong I2C address | Use I2C scanner to find correct address |
| Card not reading | Card not supported or incorrect wiring | Verify card type and connections |
| Servo not moving | Power issue or incorrect pin | Check power supply and pin configuration |
| MQTT connection failing | Network or credential issues | Verify WiFi connection and MQTT settings |

## Code Structure

- **Setup**: Initializes hardware components and connections
- **Main Loop**: Handles card reading and access decisions
- **Admin Functions**: Manages card database operations
- **MQTT Functions**: Handles communication with the cloud
- **Ultrasonic Functions**: Processes sensor data for entry detection
- **Utilities**: Helper functions for various operations

## Future Enhancements

- **Fingerprint Integration**: Add biometric authentication
- **Multi-Factor Authentication**: Require multiple forms of identification
- **Time-Based Access**: Restrict access based on schedules
- **User Categories**: Different access levels for different users
- **Mobile App Control**: Remote management via smartphone 