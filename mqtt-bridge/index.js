require('dotenv').config();
const mqtt = require('mqtt');
const { createClient } = require('@supabase/supabase-js');
const express = require('express');

// Create Express app for health checks
const app = express();
const port = process.env.PORT || 3000;

// Health check endpoint
app.get('/', (req, res) => {
  res.json({
    status: 'healthy',
    mqtt: client.connected ? 'connected' : 'disconnected',
    uptime: process.uptime()
  });
});

// Start HTTP server
app.listen(port, () => {
  console.log(`Health check server listening on port ${port}`);
});

// MQTT Configuration
const mqttOptions = {
  host: process.env.MQTT_HOST || '0b8b2b868c8d466995ff825a840cef50.s1.eu.hivemq.cloud',
  port: parseInt(process.env.MQTT_PORT) || 8883,
  protocol: 'mqtts',
  username: process.env.MQTT_USERNAME || 'nestnest1',
  password: process.env.MQTT_PASSWORD || '1234Nest'
};

// Supabase Configuration
const supabaseUrl = process.env.SUPABASE_URL;
const supabaseKey = process.env.SUPABASE_KEY;
const supabase = createClient(supabaseUrl, supabaseKey);

// MQTT Topics
const topics = {
  temperature: 'temperature',
  humidity: 'humidity',
  fan: 'fan'
};

// Connect to MQTT broker
const client = mqtt.connect(mqttOptions);

client.on('connect', () => {
  console.log('Connected to MQTT broker');
  
  // Subscribe to all topics
  Object.values(topics).forEach(topic => {
    client.subscribe(topic, (err) => {
      if (err) {
        console.error(`Error subscribing to ${topic}:`, err);
      } else {
        console.log(`Subscribed to ${topic}`);
      }
    });
  });
});

client.on('message', async (topic, message) => {
  const value = message.toString();
  console.log(`Received message on ${topic}: ${value}`);
  
  try {
    const now = new Date().toISOString();
    const data = {
      created_at: now,
      updated_at: now,
      sensor_id: 1,  // You can adjust this based on your needs
      temperature_sensors: 1,  // You can adjust this based on your needs
      temperature: null,
      humidity: null,
      fan_status: null
    };

    // Update the relevant field based on the topic
    switch (topic) {
      case topics.temperature:
        data.temperature = parseFloat(value);
        break;
      case topics.humidity:
        data.humidity = parseFloat(value);
        break;
      case topics.fan:
        data.fan_status = value;
        break;
    }

    // Insert into temperature_readings table
    const { error } = await supabase
      .from('temperature_readings')
      .insert([data]);
      
    if (error) {
      console.error('Error inserting into temperature_readings:', error);
    } else {
      console.log('Successfully inserted data');
    }
  } catch (error) {
    console.error('Error processing message:', error);
  }
});

client.on('error', (error) => {
  console.error('MQTT Error:', error);
});

// Keep the process running
process.on('SIGTERM', () => {
  console.log('Received SIGTERM. Cleaning up...');
  client.end();
  process.exit(0);
});

process.on('SIGINT', () => {
  console.log('Received SIGINT. Cleaning up...');
  client.end();
  process.exit(0);
}); 