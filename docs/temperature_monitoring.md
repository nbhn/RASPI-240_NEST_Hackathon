# Temperature Monitoring System

## Overview

The temperature monitoring system provides real-time monitoring and alerting for industrial environments. It features high-precision sensors, adaptive thresholds, and predictive maintenance capabilities to ensure optimal operating conditions and prevent equipment failures.

## Features

- **Precision Sensing**: High-accuracy temperature sensors with ±0.5°C precision
- **Real-time Monitoring**: Continuous data collection with configurable sampling rates
- **Multi-zone Coverage**: Independent monitoring for different areas of the facility
- **Alert System**: Configurable threshold-based alerts via SMS, email, and dashboard
- **Predictive Analytics**: ML-based algorithms to predict system failures before they occur
- **Historical Data**: Comprehensive data storage for trend analysis and reporting

## Hardware Components

- DHT22/AM2302 temperature and humidity sensors
- ESP32 microcontroller for data collection
- RS485 network for industrial environments
- Custom PCB for sensor interfacing
- 4-20mA industrial sensor options for harsh environments

## Implementation

The system uses ESP32 microcontrollers to collect data from multiple sensors and transmit it via MQTT:

```cpp
// Key components from TemperatureControl.ino
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

#define DHTPIN 4     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22 (AM2302)

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Function to read and publish temperature data
void readAndPublishTemperature() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  
  // Create JSON document
  StaticJsonDocument<200> doc;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["timestamp"] = millis();
  doc["sensorId"] = SENSOR_ID;
  
  // Serialize JSON to string
  char buffer[256];
  serializeJson(doc, buffer);
  
  // Publish to MQTT
  client.publish("raspi240/temperature/data", buffer);
  
  // Check thresholds and alert if necessary
  if (temperature > HIGH_THRESHOLD || temperature < LOW_THRESHOLD) {
    doc["alert"] = true;
    doc["alertType"] = temperature > HIGH_THRESHOLD ? "high" : "low";
    serializeJson(doc, buffer);
    client.publish("raspi240/temperature/alerts", buffer);
  }
}
```

## MQTT Integration

The temperature system communicates via the following MQTT topics:

- `raspi240/temperature/data` - Regular temperature and humidity readings
- `raspi240/temperature/alerts` - Threshold breach alerts
- `raspi240/temperature/config` - Configuration updates from the server
- `raspi240/temperature/status` - System status and health checks

## Configuration Options

The temperature monitoring system can be configured through the dashboard:

- Sampling rate (30s to 1h intervals)
- Alert thresholds (high and low temperature limits)
- Notification preferences (SMS, email, dashboard)
- Sensor calibration parameters
- Data retention policies

## Data Visualization

The collected temperature data is visualized in the dashboard using:

1. Real-time temperature gauges
2. Historical trend graphs
3. Heat maps for multi-zone installations
4. Alert history and resolution tracking
5. Predictive maintenance notifications

## Integration with Failure Prediction Model

Temperature data is a critical input for the equipment failure prediction model:

1. Abnormal temperature patterns are detected
2. Historical failure data is correlated with temperature events
3. Machine learning algorithms predict potential failures
4. Preventive maintenance recommendations are generated
5. Maintenance tickets are created automatically when needed

## Installation

1. Mount sensors in appropriate locations according to the facility map
2. Connect sensors to the ESP32 controllers following the provided wiring diagram
3. Power up the system and confirm connectivity to the MQTT broker
4. Configure the sensors through the web dashboard
5. Verify data is being collected and displayed correctly 