# MQTT Bridge

## Overview

The MQTT Bridge serves as the communication backbone of the entire system, connecting the ESP32-based IoT devices to the Supabase database and web dashboard. It handles message routing, data transformation, persistence, and provides API endpoints for external integrations.

## Features

- **Bidirectional Communication**: Routes messages between IoT devices and the cloud database
- **Data Transformation**: Converts raw sensor data to structured database records
- **Message Buffering**: Ensures data integrity during connectivity issues
- **Security**: Implements TLS/SSL encryption and authentication
- **API Endpoints**: Provides RESTful API for external integrations
- **Error Handling**: Robust recovery mechanisms for connection failures
- **Data Validation**: Validates and sanitizes incoming data

## Architecture

The MQTT Bridge is built using Node.js and consists of two main components:

1. **RFID Bridge** (rfid-bridge.js): Handles access control data and security events
2. **Temperature Bridge** (index.js): Manages temperature, humidity, and environmental data

Both components share a similar architecture but are specialized for their specific data types.

## RFID Bridge

The RFID Bridge handles all access control-related communication:

### MQTT Topics

- **rfid/access**: Access log entries (granted/denied)
- **rfid/door**: Door control commands
- **rfid/admin**: Administrative commands for user management
- **rfid/security**: Security events (tailgating, tampering)

### Database Tables

- **access_logs**: Records of all access attempts
- **security_events**: Security incidents and alerts

### API Endpoints

- **/api/authorize**: Add a card to the authorized list
- **/api/revoke**: Remove a card from the authorized list
- **/api/open-door**: Remotely trigger door opening
- **/api/resolve-security-event**: Mark security events as resolved

## Temperature Bridge

The Temperature Bridge manages environmental monitoring data:

### MQTT Topics

- **temperature**: Raw temperature readings
- **humidity**: Humidity sensor data
- **fan**: Fan status and control
- **sensor/data**: Unified JSON message with all sensor data

### Database Tables

- **temperature_readings**: Temperature and humidity records
- **temperature_sensors**: Sensor metadata and configuration

### Features

- **Message Buffering**: Collects related readings before database insertion
- **Data Aggregation**: Combines data from multiple sensors
- **Health Monitoring**: Tracks sensor status and uptime

## Configuration

Both bridges are configured using environment variables:

```
# MQTT Configuration
MQTT_HOST=0b8b2b868c8d466995ff825a840cef50.s1.eu.hivemq.cloud
MQTT_PORT=8883
MQTT_USERNAME=nestnest1
MQTT_PASSWORD=1234Nest

# Supabase Configuration
SUPABASE_URL=https://your-project.supabase.co
SUPABASE_KEY=your-supabase-key

# Server Configuration
PORT=3001
```

These variables can be set in a `.env` file or through environment variables.

## Installation & Deployment

### Prerequisites

- Node.js 14.x or higher
- npm or yarn package manager
- HiveMQ Cloud account
- Supabase project

### Setup

1. Clone the repository
2. Install dependencies:
   ```bash
   cd mqtt-bridge
   npm install
   ```
3. Create a `.env` file with required configuration
4. Start the bridge:
   ```bash
   npm start
   ```

### Deployment Options

- **Docker**: Containerized deployment
- **Vercel**: Serverless deployment for API endpoints
- **Heroku**: Cloud platform deployment
- **Local Server**: Self-hosted option

## Data Flow

### Access Control Data Flow

1. ESP32 RFID reader scans a card
2. Access decision is made locally
3. Result is published to `rfid/access` topic
4. RFID Bridge receives the message
5. Data is transformed to database format
6. Record is inserted into Supabase `access_logs` table
7. Dashboard retrieves and displays the data

### Temperature Data Flow

1. ESP32 reads sensor data
2. Readings are published to respective MQTT topics
3. Temperature Bridge collects related readings
4. Data is transformed and validated
5. Records are inserted into Supabase `temperature_readings` table
6. Dashboard retrieves and displays the data

## Error Handling

The bridge implements several error handling mechanisms:

- **Connection Retries**: Automatic reconnection to MQTT and database
- **Message Queuing**: Temporary storage of messages during outages
- **Validation**: Input validation to prevent database corruption
- **Logging**: Detailed error logging for troubleshooting
- **Health Checks**: Regular system health verification

## Security Considerations

- **TLS/SSL**: Encrypted MQTT and database connections
- **Authentication**: Secure credentials for all services
- **Input Validation**: Prevention of injection attacks
- **Access Control**: Limited permissions for database access
- **Secret Management**: Secure handling of credentials

## Monitoring

The bridge provides monitoring capabilities through:

- **Health Endpoint**: `/health` endpoint returns system status
- **Logs**: Detailed operation logs
- **Error Reporting**: Structured error information
- **Uptime Tracking**: Service availability monitoring

## Code Structure

### RFID Bridge

- **MQTT Connection**: Setup and maintenance of MQTT client
- **Message Handlers**: Processing of different message types
- **Database Operations**: Supabase interactions
- **API Handlers**: RESTful API implementation
- **Utilities**: Helper functions

### Temperature Bridge

- **MQTT Client**: Connection to MQTT broker
- **Message Buffer**: Temporary storage for related readings
- **Database Operations**: Supabase data insertion
- **Health Monitoring**: System status reporting
- **Error Handling**: Robust error recovery

## Troubleshooting

| Issue | Possible Cause | Solution |
|-------|---------------|----------|
| MQTT connection failure | Network issues or invalid credentials | Check network, verify MQTT credentials |
| Database insertion errors | Invalid data or permission issues | Validate data format, check Supabase permissions |
| Missing data | Message loss or processing errors | Check MQTT QoS level, verify transformation logic |
| High latency | Network congestion or resource constraints | Optimize message size, scale resources if needed |

## Future Enhancements

- **Enhanced Caching**: More sophisticated buffering mechanisms
- **Message Prioritization**: Critical message handling first
- **Horizontal Scaling**: Load distribution across multiple instances
- **Data Compression**: Reduced bandwidth usage
- **Advanced Analytics**: Edge preprocessing for data reduction 