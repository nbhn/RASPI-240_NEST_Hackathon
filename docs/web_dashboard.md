# NEST Web Dashboard

## Overview

The NEST Web Dashboard is the central interface for monitoring and controlling the entire NEST (Networked Environmental Smart Tracking) System. It provides real-time visualization of temperature data, access control logs, security events, and equipment health status. Designed with a responsive interface, it allows administrators to manage the system from any device with a web browser.

## Features

- **Real-time Monitoring**: Live updates of temperature, humidity, and sensor data
- **Access Control Management**: Manage RFID cards and face recognition authentication
- **Security Event Tracking**: Monitor and respond to security incidents
- **Equipment Health Visualization**: View predictive maintenance forecasts with failure prediction models
- **Interactive Controls**: Remotely control doors, fans, and system settings
- **Responsive Design**: Works on desktop, tablet, and mobile devices
- **User Authentication**: Secure login with role-based access control
- **Historical Data Analysis**: View trends and patterns over time
- **3D Floor Plan Integration**: Visualize sensor locations and data on interactive 3D map

## Technology Stack

The dashboard is built using modern web technologies:

- **Frontend Framework**: React with TypeScript
- **UI Components**: Shadcn UI and Tailwind CSS
- **State Management**: React Query and Context API
- **Data Visualization**: Recharts and Three.js for 3D visualization
- **Authentication**: Supabase Auth with Face Recognition integration
- **Hosting**: Vercel
- **Backend**: Supabase for database and real-time updates

## Dashboard Sections

### Home/Overview

The main dashboard provides a high-level summary of the entire system:

- Current temperature and humidity readings
- System status indicators
- Recent access events
- Active security alerts
- Equipment health summary with predicted failure times
- Quick action buttons
- 3D facility map with sensor overlays

### Temperature Monitoring

Detailed temperature monitoring section includes:

- Real-time temperature graphs
- Historical temperature data
- Fan status and controls
- Temperature threshold settings
- Alert configuration
- Sensor status indicators
- Anomaly detection highlights

### Access Control

The access control section allows management of:

- Authorized RFID cards
- Face recognition enrollment and management
- Access logs and history
- Remote door control
- User permissions
- Access schedules
- Multi-factor authentication settings

### Security Monitoring

Security section features:

- Security event logs
- Tailgating detection status
- Live alerts and notifications
- Event resolution workflow
- Face recognition verification logs
- Security camera integration
- Incident reporting tools

### Equipment Health

The equipment health section provides:

- Predictive maintenance forecasts
- Failure probability indicators
- Estimated time to failure for each device
- Maintenance scheduling tools
- Historical failure patterns
- Maintenance history logs
- Cost savings calculator

### System Configuration

The configuration section provides:

- Sensor calibration settings
- Communication parameters
- Alert thresholds
- User management
- System backup and restore
- Firmware update options
- MQTT bridge configuration

## User Interface

The dashboard UI is designed with simplicity and usability in mind:

- **Dark/Light Mode**: User-selectable theme with NEST branding
- **Responsive Layout**: Adapts to different screen sizes
- **Accessibility**: WCAG 2.1 compliant for accessibility
- **Localization**: Support for multiple languages
- **Notification System**: Toast notifications for important events
- **Interactive 3D Model**: Navigate through facility with data overlay

## Data Flow

The dashboard connects to backend data sources through:

1. **Supabase Realtime**: For live data updates
2. **Supabase REST API**: For historical data and configuration
3. **MQTT over WebSockets**: For direct device communication (optional)
4. **Face Recognition API**: For authentication verification

### Real-time Updates

The dashboard uses Supabase's real-time capabilities to provide instant updates:

```typescript
// Example of real-time subscription to temperature data
const { data: temperatureData } = supabase
  .from('temperature_readings')
  .on('INSERT', (payload) => {
    // Update UI with new temperature reading
    updateTemperatureChart(payload.new);
    
    // Check for anomalies
    if (isAnomaly(payload.new)) {
      showAlert({
        type: 'warning',
        message: `Anomaly detected in ${payload.new.sensor_id}`
      });
    }
  })
  .subscribe();
```

## Key Components

### System Summary

The SystemSummary component provides an at-a-glance view of the system status:

- Operational status indicators
- Current temperature and humidity
- Quick statistics on access events
- Active alert count
- System health score
- Failure prediction indicators

### 3D Visualization

The NEST dashboard features an interactive 3D model of the facility:

- Real-time sensor data overlay
- Color-coded temperature zones
- Interactive navigation
- Equipment location tracking
- Alert visualization
- Historical data playback

### Temperature Monitor

The TemperatureMonitor component displays:

- Real-time temperature graphs
- Min/max/average readings
- Historical trends
- Fan status and controls
- Alert thresholds
- Predictive temperature models

### Security Monitor

The SecurityMonitor component shows:

- Active security events
- Recent access attempts
- Tailgating detection status
- Door status indicators
- Security alert log
- Face recognition verification status

### Access Log

The AccessLog component provides:

- Chronological list of access events
- Filtering by user, time, and access point
- Success/failure status
- Event details on demand
- Export capabilities
- Authentication method used (RFID, Face, or both)

## Authentication & Authorization

The dashboard implements secure access control:

- **Face Recognition**: For administrator access
- **JWT-based Authentication**: Secure token-based login
- **Role-based Access**: Different permissions for admins, operators, and viewers
- **Session Management**: Automatic timeout and refresh
- **Audit Logging**: Tracking of administrative actions
- **Multi-factor Options**: Combine face recognition with RFID or password

## Installation & Deployment

The dashboard is deployed on Vercel for reliable hosting:

1. The application is built with Vite
2. Assets are optimized during the build process
3. Deployment is triggered automatically from GitHub
4. Custom domain setup with HTTPS
5. Edge caching for improved performance

## Configuration

The dashboard is configured through environment variables:

```
VITE_SUPABASE_URL=https://your-project.supabase.co
VITE_SUPABASE_ANON_KEY=your-supabase-key
VITE_API_ENDPOINT=https://your-api.example.com
VITE_MQTT_WEBSOCKET_URL=wss://mqtt.example.com:9001
VITE_FACE_RECOGNITION_API=https://api.face.example.com
```

## Accessing the Dashboard

The live dashboard is available at:

[https://nest-dashboard.vercel.app/](https://nest-dashboard.vercel.app/)

### Demo Credentials

For demonstration purposes, use:
- **Username**: demo@example.com
- **Password**: NestDemo2025

## Mobile Compatibility

The NEST dashboard is fully responsive and works on mobile devices:

- Optimized layouts for small screens
- Touch-friendly controls
- Push notifications for alerts
- Offline capabilities for basic monitoring
- Progressive Web App (PWA) support

## Integration with Other NEST Systems

The dashboard integrates seamlessly with other NEST components:

- **Temperature Control System**: Monitor and adjust temperature settings
- **RFID Access Control**: Manage access permissions and monitor entries
- **Face Recognition System**: Enroll and manage face templates
- **Failure Prediction Model**: View equipment health and maintenance forecasts
- **MQTT Bridge**: Monitor communication status between systems

## Future Enhancements

Planned dashboard improvements include:

- **Advanced Analytics**: More detailed data analysis tools
- **Customizable Dashboard**: User-configurable layout and widgets
- **Mobile App**: Native mobile application for enhanced features
- **Multi-site Support**: Managing multiple installations from one interface
- **AI Insights**: Intelligent recommendations based on system data

## Troubleshooting

| Issue | Possible Cause | Solution |
|-------|---------------|----------|
| Dashboard not updating | WebSocket connection lost | Check network, refresh page |
| Authentication failures | Expired tokens or credentials | Log out and log in again |
| Charts not rendering | Data loading issue or browser compatibility | Check browser console, try different browser |
| Slow performance | Too much historical data | Limit date ranges, clear browser cache |

## Development

### Local Development Setup

1. Clone the repository
2. Install dependencies:
   ```bash
   npm install
   ```
3. Create a `.env.local` file with required variables
4. Start the development server:
   ```bash
   npm run dev
   ```
5. Access the dashboard at http://localhost:8080

### Building for Production

```bash
npm run build
```

This creates optimized files in the `dist` directory that can be deployed to any static hosting service.

## Code Structure

The dashboard codebase is organized into:

- **components/**: Reusable UI components
- **pages/**: Main application pages
- **hooks/**: Custom React hooks
- **services/**: API and data services
- **lib/**: Utility functions
- **contexts/**: React context providers
- **types/**: TypeScript type definitions 