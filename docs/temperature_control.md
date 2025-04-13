# Temperature Control System

## Overview

The Temperature Control System is designed to monitor and regulate environmental conditions in industrial and commercial spaces. It provides real-time temperature and humidity tracking with automated control of cooling systems, historical data analysis, and alert generation for abnormal conditions.

## Features

- **Real-time Monitoring**: Continuous tracking of temperature and humidity
- **Automated Control**: Fan and cooling system activation based on temperature thresholds
- **Data Logging**: Historical temperature and humidity data stored in Supabase
- **Alert System**: Notifications for temperature anomalies and system malfunctions
- **MQTT Integration**: Cloud connectivity for remote monitoring and control
- **Energy Efficiency**: Intelligent fan control algorithms to minimize power consumption

## Hardware Components

- **ESP32 Microcontroller**: Primary control unit
- **DS18B20 Temperature Sensors**: Digital temperature measurement
- **DHT22/DHT11 Sensors**: Humidity and backup temperature measurement
- **Relay Modules**: Fan and cooling system control
- **LEDs**: Status indicators
- **Optional LCD Display**: Local temperature and status display

## Pin Configuration

| Component | ESP32 Pin |
|-----------|-----------|
| DS18B20 Data | GPIO 4  |
| DHT22 Data   | GPIO 15 |
| Fan Relay    | GPIO 23 |
| Status LED   | GPIO 2  |
| Alert LED    | GPIO 16 |
| LCD SDA (Optional) | GPIO 21 |
| LCD SCL (Optional) | GPIO 22 |

## How It Works

### Temperature Monitoring

1. The system reads temperature data from the DS18B20 sensors at regular intervals
2. Humidity data is collected from the DHT22/DHT11 sensor
3. Data is processed and filtered to eliminate spurious readings
4. The system calculates average, minimum, and maximum values
5. Current readings are displayed locally (if LCD is connected)
6. All data is transmitted to the MQTT broker for cloud storage and visualization

### Automated Control

1. Temperature readings are compared against predefined thresholds:
   - Below minimum threshold: All cooling systems off
   - Between minimum and maximum: Proportional fan speed control
   - Above maximum threshold: Maximum cooling, alert triggered
2. Fan speed is controlled using PWM for energy efficiency
3. Hysteresis is implemented to prevent rapid cycling of cooling systems
4. Manual override is available through MQTT commands

### Alert System

1. Alerts are triggered when:
   - Temperature exceeds maximum safe threshold
   - Temperature changes too rapidly
   - Sensor readings are inconsistent or fail
   - System components malfunction
2. Alerts are sent to:
   - MQTT broker for dashboard display
   - Local indicators (LEDs, buzzer if available)
   - Optionally, email/SMS through external integration

## Software Architecture

### Main Components

- **Sensor Management**: Handles sensor initialization, reading, and validation
- **Control Logic**: Implements the temperature control algorithms
- **Data Processing**: Filters and processes sensor readings
- **MQTT Client**: Manages cloud communication
- **Alert System**: Monitors for abnormal conditions
- **Configuration**: Stores and manages system parameters

### Data Flow

1. Sensor readings → Data processing → Control logic
2. Control logic → Actuator control (fans, cooling)
3. Processed data → MQTT client → Cloud storage
4. Cloud storage → Dashboard visualization
5. Dashboard → Manual controls → MQTT client → Control logic

## Integration with MQTT Bridge

The Temperature Control System communicates with the MQTT bridge using the following topics:

- **temperature**: Current temperature readings
- **humidity**: Current humidity readings
- **fan**: Current fan status and speed
- **sensor/data**: Unified JSON message with all sensor data
- **control/fan**: Commands to control fan operation
- **control/mode**: Commands to change operating mode
- **alerts**: System alerts and warnings

## Data Format

### MQTT Messages

Temperature readings:
```
[sensor_id]:[temperature_value]
```

Humidity readings:
```
[sensor_id]:[humidity_value]
```

Fan status:
```
[fan_id]:[speed]:[mode]
```

Unified data message:
```json
{
  "temperature": 23.5,
  "humidity": 45.2,
  "fan_status": "ON",
  "fan_speed": 75,
  "timestamp": "2025-04-13T12:34:56Z"
}
```

## Configuration Options

The system can be configured through:

1. **Hardcoded Constants**: Basic parameters defined in the code
2. **EEPROM Storage**: Persistent configuration stored in ESP32 EEPROM
3. **MQTT Commands**: Dynamic configuration changes via MQTT messages

Key configuration parameters include:

- Temperature thresholds (min, max, alert)
- Sampling intervals
- Control algorithm parameters
- MQTT broker details
- WiFi credentials

## Troubleshooting

### Common Issues

| Issue | Possible Cause | Solution |
|-------|---------------|----------|
| Inaccurate readings | Sensor placement or interference | Relocate sensors away from heat sources |
| Erratic fan behavior | Noisy power supply or incorrect threshold | Stabilize power supply, adjust hysteresis |
| MQTT connection loss | Network issues or credential problems | Check WiFi signal strength, verify credentials |
| System reset loops | Power issues or code bugs | Add decoupling capacitors, check for memory leaks |

### Diagnostic Mode

The system includes a built-in diagnostic mode activated by:
1. Power cycling the device 3 times in quick succession, or
2. Sending a special MQTT command

In diagnostic mode, the system will:
- Run sensor self-tests
- Verify actuator operation
- Check MQTT connectivity
- Report all results via MQTT

## Future Enhancements

- **Multi-zone Control**: Independent temperature management for different areas
- **Machine Learning**: Adaptive control based on usage patterns
- **Web Configuration Interface**: Browser-based setup and configuration
- **Additional Sensors**: Air quality, pressure, occupancy integration
- **Battery Backup**: Uninterrupted operation during power outages

## Code Structure

The Temperature Control software is structured into several modules:

- **main.ino**: Primary program file with setup and loop functions
- **sensor.h/cpp**: Sensor management and reading functions
- **control.h/cpp**: Temperature control algorithms
- **mqtt.h/cpp**: MQTT communication functions
- **config.h/cpp**: Configuration management
- **alert.h/cpp**: Alert handling and notification 