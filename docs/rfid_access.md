# RFID Access Control System

## Overview

The RFID Access Control System provides secure, contactless entry to restricted areas while maintaining detailed access logs and preventing unauthorized access attempts. Built on ESP32 microcontrollers, the system offers standalone operation with cloud synchronization capabilities.

## Key Features

- **Secure Authentication**: Real-time validation against authorized card database
- **Anti-Tailgating Detection**: Ultrasonic sensors prevent unauthorized entry
- **Multi-factor Authentication**: Optional integration with face recognition for high-security areas
- **Cloud Synchronization**: All access logs stored in Supabase database
- **Remote Management**: Remotely add/revoke access permissions
- **Real-time Alerts**: Instant notifications for security events
- **Offline Operation**: Functions without internet connection using local card database
- **LED and LCD Feedback**: Clear visual indicators of access status

## Hardware Components

- **ESP32 WROOM Development Board**: Main control unit
- **MFRC522 RFID Reader**: 13.56MHz RFID card reading
- **HC-SR04 Ultrasonic Sensor**: Tailgating detection
- **16x2 I2C LCD Display**: User information and status display
- **LEDs & Buzzer**: Visual and audio indicators
- **Servo Motor**: Door lock mechanism
- **Push Button**: Admin mode access

## System Architecture

The RFID system employs a hybrid architecture:

1. **Local Processing**: 
   - Card validation happens on the ESP32
   - Local database stored in EEPROM/SPIFFS
   - Anti-tailgating logic runs locally

2. **Cloud Integration**:
   - Access logs sent to Supabase via MQTT Bridge
   - New card authorizations received from cloud
   - Remote management through web dashboard

## Main Features Explained

### Card Authentication

The system uses the MFRC522 reader to capture card UIDs and validate them against an authorized list:

```cpp
bool authenticateCard(byte* cardUID) {
  // Check if card is in the authorized list
  for (int i = 0; i < authorizedCardCount; i++) {
    if (memcmp(authorizedCards[i], cardUID, 4) == 0) {
      return true;
    }
  }
  return false;
}
```

### Anti-Tailgating System

The ultrasonic sensor monitors the doorway after access is granted to prevent multiple entries:

```cpp
void monitorDoorway() {
  long duration, distance;
  int detectedChanges = 0;
  
  // Check for multiple entries within validation window
  unsigned long startTime = millis();
  while (millis() - startTime < DOOR_OPEN_TIME) {
    // Measure distance
    digitalWrite(TRIGGER_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGGER_PIN, LOW);
    
    duration = pulseIn(ECHO_PIN, HIGH);
    distance = (duration / 2) / 29.1;
    
    // Detect significant changes in distance
    if (abs(distance - lastDistance) > MOVEMENT_THRESHOLD) {
      detectedChanges++;
      delay(100);
    }
    
    lastDistance = distance;
    delay(50);
  }
  
  // If multiple movements detected, possible tailgating
  if (detectedChanges > TAILGATING_THRESHOLD) {
    triggerSecurityAlert("Possible tailgating detected");
  }
}
```

### Access Logging

All access events are logged locally and synchronized with the cloud:

```cpp
void logAccessEvent(byte* cardUID, bool accessGranted) {
  // Prepare JSON data
  StaticJsonDocument<256> doc;
  doc["card_uid"] = uidToString(cardUID);
  doc["timestamp"] = getCurrentTimestamp();
  doc["granted"] = accessGranted;
  doc["reader_id"] = READER_ID;
  
  // Serialize to JSON
  char buffer[256];
  serializeJson(doc, buffer);
  
  // Publish to MQTT
  mqttClient.publish("rfid/access", buffer);
  
  // Also store locally
  storeLocalLog(doc);
}
```

### Remote Management

The system can receive updates to the authorized card list via MQTT:

```cpp
void handleMqttMessage(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, "rfid/admin/add") == 0) {
    // Add new card
    StaticJsonDocument<256> doc;
    deserializeJson(doc, payload, length);
    
    byte newCardUID[4];
    hexStringToByteArray(doc["card_uid"].as<String>(), newCardUID);
    addCardToAuthorizedList(newCardUID);
    
  } else if (strcmp(topic, "rfid/admin/remove") == 0) {
    // Remove card
    StaticJsonDocument<256> doc;
    deserializeJson(doc, payload, length);
    
    byte cardToRemove[4];
    hexStringToByteArray(doc["card_uid"].as<String>(), cardToRemove);
    removeCardFromAuthorizedList(cardToRemove);
  }
}
```

## Integration Points

### MQTT Integration

The RFID system communicates with the cloud using these MQTT topics:

- **rfid/access**: Access events (publish)
- **rfid/security**: Security alerts (publish)
- **rfid/admin/add**: Add card authorization (subscribe)
- **rfid/admin/remove**: Remove card authorization (subscribe)
- **rfid/admin/door**: Remote door control (subscribe)
- **rfid/status**: System status updates (publish)

### Web Dashboard Integration

The RFID system integrates with the web dashboard for:

- Viewing access logs
- Managing authorized cards
- Monitoring security events
- Remote door control
- System status monitoring

### Face Recognition Integration

For high-security areas, the RFID system can be paired with face recognition:

1. User presents RFID card
2. System performs initial authentication
3. If card is valid, face recognition is activated
4. Access is granted only if both authenticate successfully

## Installation Guide

1. Connect hardware components according to the pin configuration
2. Flash the ESP32 with the provided firmware
3. Configure WiFi and MQTT settings through the serial console
4. Test basic card reading functionality
5. Configure tailgating sensitivity through the web interface
6. Add authorized cards through admin mode or web dashboard

## Troubleshooting

| Issue | Possible Cause | Solution |
|-------|---------------|----------|
| Card not reading | Incorrect wiring | Check MFRC522 SPI connections |
| Inconsistent readings | Power supply issues | Use separate power supply for servo |
| Door not unlocking | Servo configuration | Adjust servo min/max positions |
| Cannot connect to MQTT | Network issues | Check WiFi signal strength |
| LCD not displaying | I2C address wrong | Run I2C scanner to find correct address |

## Security Recommendations

- Mount the control unit in a secure, tamper-resistant enclosure
- Use encrypted communication for all MQTT messages
- Regularly update the authorized card list
- Enable multi-factor authentication for sensitive areas
- Implement regular security audits and penetration testing

## Future Enhancements

- **NFC Mobile Support**: Use smartphones as access credentials
- **Biometric Integration**: Fingerprint reader support
- **Battery Backup**: Continued operation during power outages
- **Visitor Management**: Temporary access credential creation
- **Occupancy Tracking**: Count personnel in restricted areas 