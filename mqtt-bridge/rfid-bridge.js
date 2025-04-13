require('dotenv').config();
const mqtt = require('mqtt');
const fetch = require('node-fetch');
const { createClient } = require('@supabase/supabase-js');
const express = require('express');

// Make fetch available globally for Supabase client
global.fetch = fetch;

// Create Express app for health checks and API
const app = express();
const port = process.env.PORT || 3001;

// Enable CORS for all routes
app.use((req, res, next) => {
  res.header('Access-Control-Allow-Origin', '*');
  res.header('Access-Control-Allow-Headers', 'Origin, X-Requested-With, Content-Type, Accept');
  res.header('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS');
  
  // Handle preflight requests
  if (req.method === 'OPTIONS') {
    return res.sendStatus(200);
  }
  next();
});

app.use(express.json());

// MQTT Configuration
const mqttOptions = {
  host: process.env.MQTT_HOST || '0b8b2b868c8d466995ff825a840cef50.s1.eu.hivemq.cloud',
  port: parseInt(process.env.MQTT_PORT) || 8883,
  protocol: 'mqtts',
  username: process.env.MQTT_USERNAME || 'nestnest1',
  password: process.env.MQTT_PASSWORD || '1234Nest'
};

// Supabase Configuration
const supabaseUrl = (process.env.SUPABASE_URL || 'https://example.supabase.co').replace(/\/$/, '');
const supabaseKey = process.env.SUPABASE_KEY || 'your-anon-key';
const supabase = createClient(supabaseUrl, supabaseKey);

console.log(`Connecting to Supabase at: ${supabaseUrl}`);

// MQTT Topics
const topics = {
  accessLog: 'rfid/access',
  doorControl: 'rfid/door',
  admin: 'rfid/admin',
  security: 'rfid/security'
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

  // Check for card authorizations on startup
  syncAuthorizedCards();
});

// Process incoming MQTT messages
client.on('message', async (topic, message) => {
  const value = message.toString();
  console.log(`Received message on ${topic}: ${value}`);
  
  try {
    // Handle different topics
    if (topic === topics.accessLog) {
      await handleAccessLog(value);
    } else if (topic === topics.admin) {
      await handleAdminCommand(value);
    } else if (topic === topics.security) {
      await handleSecurityEvent(value);
    }
  } catch (error) {
    console.error('Error processing message:', error);
  }
});

// Handle access log entries
async function handleAccessLog(message) {
  // Example from ESP32: "GRANTED:33:06:73:0E:Main Door" or "DENIED:33:06:73:0E:Main Door"
  try {
    console.log(`Processing access log: ${message}`);
    
    const parts = message.split(':');
    if (parts.length < 5) {
      console.error('Invalid message format:', message);
      return;
    }
    
    const status = parts[0];  // GRANTED or DENIED
    
    // Handle RFID tag which is in format XX:XX:XX:XX
    const rfidTag = `${parts[1]}:${parts[2]}:${parts[3]}:${parts[4]}`;
    
    // Extract user name and access point if available, otherwise use defaults
    let userName = 'Unknown';
    let accessPoint = 'Main Door';
    
    if (parts.length > 5) {
      accessPoint = parts[5];
    }
    
    // Look up user name from previous logs if available
    try {
      const { data, error } = await supabase
        .from('access_logs')
        .select('user_name')
        .eq('rfid_tag', rfidTag)
        .not('user_name', 'eq', 'Unknown')
        .order('created_at', { ascending: false })
        .limit(1);
        
      if (!error && data && data.length > 0 && data[0].user_name) {
        userName = data[0].user_name;
      }
    } catch (err) {
      console.error('Error looking up user name:', err);
    }

    console.log(`Logging access: ${status} for ${userName} (${rfidTag}) at ${accessPoint}`);
    
    // Create current timestamp in ISO format
    const timestamp = new Date().toISOString();
    
    // Insert access log into Supabase
    const { error } = await supabase
      .from('access_logs')
      .insert([{
        user_name: userName,
        rfid_tag: rfidTag,
        access_point: accessPoint,
        status: status,
        created_at: timestamp
      }]);
      
    if (error) {
      console.error('Error inserting access log:', error);
    } else {
      console.log(`Successfully logged access attempt at ${timestamp}`);
    }
  } catch (error) {
    console.error('Error processing access log:', error);
  }
}

// Handle admin commands
async function handleAdminCommand(message) {
  try {
    if (message.startsWith('CARD_ADDED:') || message.startsWith('CARD_REMOVED:')) {
      const action = message.startsWith('CARD_ADDED:') ? 'added' : 'removed';
      const rfidTag = message.substring(message.indexOf(':') + 1);
      console.log(`Card ${action}: ${rfidTag}`);
      
      // Update Supabase with the new/removed card
      if (action === 'added') {
        await updateCardInSupabase(rfidTag, true);
      } else {
        await updateCardInSupabase(rfidTag, false);
      }
    }
  } catch (error) {
    console.error('Error handling admin command:', error);
  }
}

// Sync card authorizations with Supabase
async function syncAuthorizedCards() {
  try {
    // Get all authorized cards from Supabase
    const { data, error } = await supabase
      .from('access_logs')
      .select('rfid_tag, user_name')
      .eq('status', 'AUTHORIZED');
      
    if (error) {
      console.error('Error fetching authorized cards:', error);
      return;
    }
    
    // Send ADD commands for each authorized card
    if (data && data.length > 0) {
      console.log(`Syncing ${data.length} authorized cards...`);
      
      data.forEach(card => {
        const command = `ADD:${card.rfid_tag}`;
        client.publish(topics.admin, command);
        console.log(`Sent authorization for: ${card.rfid_tag} (${card.user_name || 'Unknown'})`);
      });
    } else {
      console.log('No authorized cards found in database');
    }
  } catch (error) {
    console.error('Error syncing authorized cards:', error);
  }
}

// Update card in Supabase
async function updateCardInSupabase(rfidTag, isAuthorized) {
  try {
    if (isAuthorized) {
      // Check if card exists in logs but is not authorized
      const { data, error } = await supabase
        .from('access_logs')
        .select('id, user_name')
        .eq('rfid_tag', rfidTag)
        .order('created_at', { ascending: false })
        .limit(1);
      
      if (error) {
        console.error('Error checking card:', error);
        return;
      }
      
      // Add new authorized card entry
      const userName = data && data.length > 0 ? data[0].user_name : 'Unknown User';
      const { error: insertError } = await supabase
        .from('access_logs')
        .insert([{
          user_name: userName,
          rfid_tag: rfidTag,
          access_point: 'System',
          status: 'AUTHORIZED',
          created_at: new Date().toISOString()
        }]);
        
      if (insertError) {
        console.error('Error inserting authorized card:', insertError);
      } else {
        console.log(`Card ${rfidTag} authorized in Supabase`);
      }
    } else {
      // Revoke authorization - update status to REVOKED
      const { error } = await supabase
        .from('access_logs')
        .update({ 
          status: 'REVOKED'
        })
        .eq('rfid_tag', rfidTag)
        .eq('status', 'AUTHORIZED');
        
      if (error) {
        console.error('Error revoking card:', error);
      } else {
        console.log(`Card ${rfidTag} revoked in Supabase`);
      }
    }
  } catch (error) {
    console.error('Error updating card in Supabase:', error);
  }
}

// Handle security events
async function handleSecurityEvent(message) {
  try {
    console.log(`Processing security event: ${message}`);
    
    // Parse the JSON message
    let securityEvent;
    try {
      securityEvent = JSON.parse(message);
    } catch (err) {
      console.error('Invalid security event JSON:', err);
      return;
    }
    
    // Validate required fields
    if (!securityEvent.sensor_id || !securityEvent.event_type || !securityEvent.description) {
      console.error('Missing required fields in security event:', securityEvent);
      return;
    }
    
    // Create current timestamp in ISO format
    const timestamp = new Date().toISOString();
    
    // Insert security event into Supabase
    const { data, error } = await supabase
      .from('security_events')
      .insert([{
        sensor_id: securityEvent.sensor_id,
        event_type: securityEvent.event_type,
        description: securityEvent.description,
        severity: securityEvent.severity || 'medium',
        status: securityEvent.status || 'active',
        duration: securityEvent.duration || 0,
        security_sensors: securityEvent.security_sensors || securityEvent.sensor_id,
        created_at: timestamp,
        resolved_at: null
      }]);
      
    if (error) {
      console.error('Error inserting security event:', error);
    } else {
      console.log(`Successfully logged security event at ${timestamp}`);
    }
  } catch (error) {
    console.error('Error processing security event:', error);
  }
}

// API Endpoints
app.get('/health', (req, res) => {
  res.json({
    status: 'healthy',
    mqtt: client.connected ? 'connected' : 'disconnected',
    uptime: process.uptime()
  });
});

// API Endpoint to authorize a card
app.post('/api/authorize', async (req, res) => {
  try {
    const { rfid_tag, user_name } = req.body;
    
    if (!rfid_tag) {
      return res.status(400).json({ error: 'RFID tag is required' });
    }
    
    // Add card to authorized list
    const command = `ADD:${rfid_tag}`;
    client.publish(topics.admin, command);
    
    // Update Supabase
    const { error } = await supabase
      .from('access_logs')
      .insert([{
        user_name: user_name || 'Unknown User',
        rfid_tag: rfid_tag,
        access_point: 'API',
        status: 'AUTHORIZED',
        created_at: new Date().toISOString()
      }]);
      
    if (error) {
      console.error('Error authorizing card via API:', error);
      return res.status(500).json({ error: 'Database error' });
    }
    
    res.json({ success: true, message: `Card ${rfid_tag} authorized` });
  } catch (error) {
    console.error('API error:', error);
    res.status(500).json({ error: 'Server error' });
  }
});

// API Endpoint to revoke a card
app.post('/api/revoke', async (req, res) => {
  try {
    const { rfid_tag } = req.body;
    
    if (!rfid_tag) {
      return res.status(400).json({ error: 'RFID tag is required' });
    }
    
    // Remove card from authorized list
    const command = `REMOVE:${rfid_tag}`;
    client.publish(topics.admin, command);
    
    // Update Supabase
    const { error } = await supabase
      .from('access_logs')
      .update({ 
        status: 'REVOKED'
      })
      .eq('rfid_tag', rfid_tag)
      .eq('status', 'AUTHORIZED');
      
    if (error) {
      console.error('Error revoking card via API:', error);
      return res.status(500).json({ error: 'Database error' });
    }
    
    res.json({ success: true, message: `Card ${rfid_tag} revoked` });
  } catch (error) {
    console.error('API error:', error);
    res.status(500).json({ error: 'Server error' });
  }
});

// API Endpoint to open door remotely
app.post('/api/open-door', (req, res) => {
  try {
    const { door_id } = req.body;
    client.publish(topics.doorControl, 'OPEN');
    res.json({ success: true, message: `Door ${door_id || 'main'} opened` });
  } catch (error) {
    console.error('API error:', error);
    res.status(500).json({ error: 'Server error' });
  }
});

// Add API endpoint to resolve security events
app.post('/api/resolve-security-event', async (req, res) => {
  try {
    const { event_id, resolution_notes } = req.body;
    
    if (!event_id) {
      return res.status(400).json({ error: 'Event ID is required' });
    }
    
    // Update the security event in Supabase
    const { data, error } = await supabase
      .from('security_events')
      .update({ 
        status: 'resolved',
        resolved_at: new Date().toISOString(),
        resolution_notes: resolution_notes || 'Resolved by admin'
      })
      .eq('id', event_id);
      
    if (error) {
      console.error('Error resolving security event:', error);
      return res.status(500).json({ error: 'Database error' });
    }
    
    res.json({ success: true, message: `Security event ${event_id} resolved` });
  } catch (error) {
    console.error('API error:', error);
    res.status(500).json({ error: 'Server error' });
  }
});

// Start HTTP server
app.listen(port, () => {
  console.log(`RFID bridge server listening on port ${port}`);
});

// Error handling
client.on('error', (error) => {
  console.error('MQTT Error:', error);
});

// Graceful shutdown
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