# Installation Guide

This document provides detailed setup instructions for the Smart Industrial Monitoring System developed by Team RASPI-240.

## Table of Contents
- [Prerequisites](#prerequisites)
- [Hardware Setup](#hardware-setup)
- [Software Installation](#software-installation)
- [MQTT Bridge Configuration](#mqtt-bridge-configuration)
- [Database Setup](#database-setup)
- [Web Dashboard Deployment](#web-dashboard-deployment)
- [Testing the System](#testing-the-system)
- [Troubleshooting](#troubleshooting)

## Prerequisites

### Hardware Requirements
- 2× ESP32 WROOM Model S Development Boards
- RFID Reader Module and Tags
- DHT22 Temperature/Humidity Sensor
- 2× 16×2 LCD Displays
- Ultrasonic Sensor
- Servo Motor
- 2× 24V Cooling Fans
- LEDs, Resistors, Jumper Wires
- Breadboards or PCBs
- Power Adapters

### Software Requirements
- Arduino IDE (2.0 or newer)
- Node.js (v14.0.0 or newer)
- Git
- Supabase Account
- HiveMQ Cloud Account (Free tier is sufficient for testing)

## Hardware Setup

### Access Control Unit
1. **ESP32 Connections**:
   - Connect RFID Reader: 
     - SDA → GPIO21
     - SCK → GPIO18
     - MOSI → GPIO23
     - MISO → GPIO19
     - RST → GPIO22
     - 3.3V and GND to respective pins
   - Connect LCD Display:
     - SDA → GPIO13
     - SCL → GPIO14
     - VCC → 5V
     - GND → GND
   - Connect Ultrasonic Sensor:
     - TRIG → GPIO27
     - ECHO → GPIO26
     - VCC → 5V
     - GND → GND
   - Connect Servo Motor:
     - Signal → GPIO25
     - VCC → External 5V (not from ESP32)
     - GND → Common GND
   - Connect LEDs:
     - Green LED → GPIO32 (with 220Ω resistor)
     - Red LED → GPIO33 (with 220Ω resistor)

2. **Power Supply**:
   - Use a stable 5V power supply capable of delivering at least 2A
   - Ensure common ground between ESP32 and all components

### Temperature Monitoring Unit
1. **ESP32 Connections**:
   - Connect DHT22 Sensor:
     - Data → GPIO15
     - VCC → 3.3V
     - GND → GND
   - Connect LCD Display:
     - SDA → GPIO21
     - SCL → GPIO22
     - VCC → 5V
     - GND → GND
   - Connect Fans (via transistors or relays):
     - Fan 1 Control → GPIO26
     - Fan 2 Control → GPIO27
     - VCC → External 24V
     - GND → Common GND

2. **Power Supply**:
   - Use a stable 5V power supply for ESP32
   - Use 24V power supply for fans
   - Ensure proper grounding between all components

## Software Installation

### 1. ESP32 Setup
1. Install ESP32 board in Arduino IDE:
   ```
   File → Preferences → Additional Boards Manager URLs → Add:
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
2. Install required libraries:
   ```
   Tools → Manage Libraries → Install:
   - MFRC522 (for RFID)
   - DHT sensor library
   - LiquidCrystal_I2C
   - PubSubClient (for MQTT)
   - ArduinoJson
   - ESP32Servo
   ```

### 2. Clone the Repository
```bash
git clone https://github.com/nbhn/RASPI-240_NEST_Hackathon.git
cd RASPI-240_NEST_Hackathon
```

### 3. Access Control Firmware
1. Open `RFIDAccessControl/RFIDAccessControl.ino` in Arduino IDE
2. Update WiFi and MQTT configuration:
   ```cpp
   // Update these with your credentials
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   const char* mqtt_server = "YOUR_MQTT_BROKER";
   const char* mqtt_username = "YOUR_MQTT_USERNAME";
   const char* mqtt_password = "YOUR_MQTT_PASSWORD";
   ```
3. Upload to the ESP32 connected to the RFID system

### 4. Temperature Control Firmware
1. Open `TemperatureControl/TemperatureControl.ino` in Arduino IDE
2. Update WiFi and MQTT configuration (similar to Access Control)
3. Set temperature thresholds if needed:
   ```cpp
   const float TEMP_THRESHOLD_HIGH = 30.0; // Adjust as needed
   const float TEMP_THRESHOLD_LOW = 25.0;  // Adjust as needed
   ```
4. Upload to the ESP32 connected to the temperature sensors

## MQTT Bridge Configuration

1. Navigate to the MQTT Bridge directory:
   ```bash
   cd mqtt-bridge
   ```

2. Install dependencies:
   ```bash
   npm install
   ```

3. Create a `.env` file with the following content:
   ```
   MQTT_HOST=your_hivemq_host.s1.eu.hivemq.cloud
   MQTT_PORT=8883
   MQTT_USERNAME=your_mqtt_username
   MQTT_PASSWORD=your_mqtt_password
   SUPABASE_URL=https://your-project-id.supabase.co
   SUPABASE_KEY=your_supabase_anon_key
   PORT=3001
   ```

4. Start the MQTT Bridge:
   ```bash
   npm start
   ```

## Database Setup

1. Create a Supabase account at [supabase.com](https://supabase.com) if you don't have one

2. Create a new project and note your project URL and anon key

3. Execute the following SQL in the Supabase SQL Editor to create required tables:

```sql
-- Access Logs Table
CREATE TABLE access_logs (
  id SERIAL PRIMARY KEY,
  user_name VARCHAR(255) NOT NULL,
  rfid_tag VARCHAR(255) NOT NULL,
  access_point VARCHAR(255) NOT NULL,
  status VARCHAR(50) NOT NULL,
  created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Temperature Readings Table
CREATE TABLE temperature_readings (
  id SERIAL PRIMARY KEY,
  sensor_id VARCHAR(255) NOT NULL,
  temperature FLOAT NOT NULL,
  humidity FLOAT,
  location VARCHAR(255),
  alert BOOLEAN DEFAULT FALSE,
  created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Security Events Table
CREATE TABLE security_events (
  id SERIAL PRIMARY KEY,
  sensor_id VARCHAR(255) NOT NULL,
  event_type VARCHAR(255) NOT NULL,
  description TEXT,
  severity VARCHAR(50) DEFAULT 'medium',
  status VARCHAR(50) DEFAULT 'active',
  duration INTEGER DEFAULT 0,
  security_sensors VARCHAR(255),
  created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
  resolved_at TIMESTAMP WITH TIME ZONE
);
```

## Web Dashboard Deployment

1. If you plan to use the web dashboard, navigate to the frontend directory:
   ```bash
   cd web-dashboard  # Adjust path if different
   ```

2. Install dependencies:
   ```bash
   npm install
   ```

3. Create a `.env` file with the Supabase configuration:
   ```
   VITE_SUPABASE_URL=https://your-project-id.supabase.co
   VITE_SUPABASE_ANON_KEY=your_supabase_anon_key
   ```

4. Start the development server:
   ```bash
   npm run dev
   ```

5. For production deployment, build and deploy:
   ```bash
   npm run build
   ```
   Then deploy the contents of the `dist` folder to your hosting service of choice (e.g., Vercel, Netlify)

## Testing the System

### Access Control Testing
1. Power on the Access Control Unit
2. Verify LCD displays "Ready for Access"
3. Present an RFID card to the reader
4. System should authenticate and display status
5. Check Supabase database for new access log entries

### Temperature Monitoring Testing
1. Power on the Temperature Monitoring Unit
2. Verify LCD displays current temperature and humidity
3. Adjust room temperature or apply heat/cold to sensor
4. Verify fans activate when temperature exceeds threshold
5. Check Supabase database for temperature readings

### MQTT Bridge Testing
1. Ensure the MQTT Bridge is running
2. Use an MQTT client to publish test messages:
   ```
   Topic: rfid/access
   Message: GRANTED:01:02:03:04:Main Door
   ```
3. Verify data appears in Supabase tables

## Troubleshooting

### ESP32 Connection Issues
- Ensure correct WiFi credentials
- Check MQTT broker address and credentials
- Verify ESP32 has sufficient power
- Monitor serial output for debugging information

### MQTT Bridge Issues
- Verify HiveMQ cloud service status
- Check environment variables in `.env` file
- Review logs for connection errors
- Ensure ports are not blocked by firewall

### Sensor Problems
- DHT22: Allow 2 seconds between readings
- RFID Reader: Keep metallic objects away from antenna
- LCDs: Check I2C address is correct (usually 0x27 or 0x3F)

### Database Issues
- Verify Supabase URL and key
- Check table schema matches expected format
- Review Supabase logs for failed operations

---

For additional support or questions, please contact Team RASPI-240 or open an issue on GitHub. 