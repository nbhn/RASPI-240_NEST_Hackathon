#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <LiquidCrystal_I2C.h>
#include "access_database.h"
#include <ESP32Servo.h>
// Pin definitions for ESP32 WROOM 32
#define RST_PIN         4           // Reset pin (changed from 22 to avoid LCD conflict)
#define SS_PIN          5           // SS/SDA pin (changed from 21 to avoid LCD conflict)
#define SERVO_PIN       26          // Door/lock relay pin
#define BUZZER_PIN      25          // Buzzer for feedback
#define GREEN_LED_PIN   32          // Access granted LED
#define RED_LED_PIN     33          // Access denied LED
#define ADMIN_BUTTON    27          // Button to enter admin mode

// Ultrasonic sensor pins
#define ULTRASONIC_TRIG_PIN 15      // Trigger pin for ultrasonic sensor
#define ULTRASONIC_ECHO_PIN 13      // Echo pin for ultrasonic sensor

// Ultrasonic sensor constants and variables
#define DOOR_MONITOR_TIMEOUT 5000   // Time to monitor door after access (5 seconds)
#define PERSON_DISTANCE_THRESHOLD 80 // Distance in cm to detect a person
#define PERSON_PASS_TIMEOUT 3000    // Time window to detect person passing (3 seconds)
#define TAILGATING_TIMEOUT 5000     // Time to monitor for tailgating (5 seconds)
#define MIN_DETECTION_INTERVAL 1500 // Minimum time between detections (1500ms)
#define DISTANCE_CHANGE_THRESHOLD 15 // Minimum distance change to count as movement

// Variables for ultrasonic detection
float lastDistance = 0;
bool personDetected = false;
bool personEntered = false;
int personCount = 0;
unsigned long entryMonitorStartTime = 0;
bool monitoringEntry = false;
bool monitoringTailgating = false;
String lastScannedCardUID = ""; // Store the last card UID for monitoring
bool ultrasonicTestMode = false; // For sensor calibration and testing
unsigned long lastDetectionTime = 0; // For debouncing
float minDistance = 1000; // For tracking minimum distance during detection
float maxDistance = 0;    // For tracking maximum distance during detection
bool inDetectionState = false; // To track if currently in detection state

/* ESP32 WROOM 32 SPI pins (already set in hardware):
 * MOSI: GPIO 23
 * MISO: GPIO 19
 * SCK:  GPIO 18
 * 
 * ESP32 WROOM 32 I2C pins for LCD (already set in hardware):
 * SDA: GPIO 21 - Used by LCD display
 * SCL: GPIO 22 - Used by LCD display
 * 
 * Note: We moved the RFID reader pins to avoid conflicts:
 * - RST_PIN changed from 22 to 4
 * - SS_PIN changed from 21 to 5
 */

// WiFi credentials
const char* ssid = "nbh";
const char* password = "11111111";

// MQTT Configuration
const char* mqtt_server = "0b8b2b868c8d466995ff825a840cef50.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;  // Using secure port 8883
const char* mqtt_username = "nestnest1";
const char* mqtt_password = "1234Nest";

// MQTT Topics
const char* access_log_topic = "rfid/access";
const char* door_control_topic = "rfid/door";
const char* admin_topic = "rfid/admin";
const char* security_topic = "rfid/security"; // Topic for security events

// Sensor IDs for the security events
#define ULTRASONIC_SENSOR_ID 1

// Security event types
#define EVENT_TYPE_TAILGATING "tailgating"
#define EVENT_TYPE_NO_ENTRY "no_entry"
#define EVENT_TYPE_TAMPERING "tampering"
#define EVENT_TYPE_SUSPICIOUS "suspicious"

// Security event severities
#define SEVERITY_LOW "low"
#define SEVERITY_MEDIUM "medium"
#define SEVERITY_HIGH "high"
#define SEVERITY_CRITICAL "critical"

// Initialize components
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Try different LCD addresses if 0x27 doesn't work
// Common addresses: 0x27, 0x3F, 0x20, 0x38
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Using 0x27 as confirmed by the I2C scanner
Servo doorServo;  
WiFiClientSecure espClient;
PubSubClient client(espClient);
AccessDatabase cardDB;

// System state
bool doorOpen = false;
unsigned long doorOpenTime = 0;
const unsigned long DOOR_OPEN_DURATION = 5000; // Door stays open for 5 seconds

// For blinking the red LED after unauthorized access
bool unauthorizedAccessDetected = false;
unsigned long lastRedLEDBlinkTime = 0;
unsigned long unauthorizedWarningStartTime = 0;
const unsigned long RED_LED_BLINK_INTERVAL = 500; // Blink every 500ms
const unsigned long UNAUTHORIZED_WARNING_DURATION = 30000; // Warning lasts for 30 seconds

// Admin mode
bool adminMode = false;
unsigned long adminModeStartTime = 0;
const unsigned long ADMIN_MODE_TIMEOUT = 30000; // Admin mode timeout after 30 seconds
byte lastScannedCard[4] = {0, 0, 0, 0};
bool cardWasScanned = false;

// Function to test different LCD addresses - returns true if found
bool findLCD() {
  byte addresses[] = {0x27, 0x3F, 0x20, 0x38};
  
  for (byte i = 0; i < 4; i++) {
    Wire.beginTransmission(addresses[i]);
    byte error = Wire.endTransmission();
    
    if (error == 0) {
      // Found LCD at this address
      Serial.print(F("LCD found at address 0x"));
      Serial.println(addresses[i], HEX);
      
      // If it's not the default address we tried, update the lcd object
      if (addresses[i] != 0x27) {
        lcd = LiquidCrystal_I2C(addresses[i], 16, 2);
      }
      
      return true;
    }
  }
  
  Serial.println(F("No LCD found at any common address!"));
  return false;
}

// Add function to safely print to LCD to avoid garbled text
void printToLCD(String line1, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  if (line2.length() > 0) {
    lcd.setCursor(0, 1);
    lcd.print(line2);
  }
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  
  // Initialize I2C with explicit pin configuration for ESP32
  Wire.begin(21, 22);  // SDA = 21, SCL = 22 on ESP32
  delay(100);  // Add delay after I2C initialization
  
  // Try to find LCD at one of the common addresses
  if (!findLCD()) {
    Serial.println(F("WARNING: LCD not found, continuing without display"));
  }
  
  // Initialize SPI bus
  SPI.begin();
  
  // Initialize MFRC522
  mfrc522.PCD_Init();
  delay(4);
  mfrc522.PCD_DumpVersionToSerial();
  
  // Initialize EEPROM database
  cardDB.begin();
  
  // Initialize LCD with extra recovery steps
  for (int i = 0; i < 3; i++) {
    lcd.init();     // Try initialization multiple times
    delay(50);
  }
  lcd.clear();
  lcd.backlight();
  delay(250);       // Add longer delay for LCD to stabilize
  printToLCD(F("RFID Access"), F("System Ready"));
  
  // Configure pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(ADMIN_BUTTON, INPUT_PULLUP);
  
  // Initialize servo ONCE here
  if (!doorServo.attached()) {
      doorServo.attach(SERVO_PIN);
  }
  doorServo.write(90); // Start in Lock position (assuming 90 is lock)
  Serial.println(F("Servo initialized and set to locked position (90)."));

  // Test servo briefly (optional, can be commented out after verification)
  // Serial.println(F("Testing servo..."));
  // doorServo.write(0); // Unlock position
  // delay(1000);
  // doorServo.write(90);  // Lock position
  // Serial.println(F("Servo test complete"));

  // Set initial LED state
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  
  // Connect to WiFi
  setup_wifi();
  
  // Setup MQTT with SSL/TLS using insecure mode (no certificate validation)
  espClient.setInsecure(); 
  
  // Set longer timeout for MQTT client
  client.setSocketTimeout(10); // 10 seconds socket timeout
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  // Attempt initial MQTT connection
  if (WiFi.status() == WL_CONNECTED) {
    reconnect();
  }
  
  Serial.println(F("RFID Access Control System Ready"));
  Serial.print(F("Number of authorized cards: "));
  Serial.println(cardDB.getCardCount());
  
  // Initialize ultrasonic sensor
  setupUltrasonicSensor();
  
  // Combination to enter ultrasonic test mode: Press and hold admin button during startup
  if (digitalRead(ADMIN_BUTTON) == LOW) {
    delay(2000); // Wait to confirm long press
    if (digitalRead(ADMIN_BUTTON) == LOW) {
      enterUltrasonicTestMode();
      while (digitalRead(ADMIN_BUTTON) == LOW) {
        delay(10); // Wait for button release 
      }
    }
  }
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print(F("Connecting to "));
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
    printToLCD(F("Connecting WiFi"), F(""));
  }

  Serial.println(F(""));
  Serial.println(F("WiFi connected"));
  Serial.println(F("IP address: "));
  Serial.println(WiFi.localIP());
  
  printToLCD(F("WiFi Connected"), F(""));
  delay(1000);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print(F("Message arrived ["));
  Serial.print(topic);
  Serial.print(F("] "));
  
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);
  
  // Handle remote door control
  if (String(topic) == door_control_topic) {
    if (message == "OPEN") {
      grantAccess(F("Remote Control"));
    } else if (message == "DIRECT_OPEN") {
      // Directly open the door without checking permissions
      openDoor();
      printToLCD(F("Door Opened"), F("Direct Command"));
      
      if (client.connected()) {
        client.publish(access_log_topic, "DIRECT_OPEN:API");
      }
    }
  }
  
  // Handle admin commands
  if (String(topic) == admin_topic) {
    if (message.startsWith("ADD:")) {
      // Format: "ADD:01:02:03:04"
      processAdminCommand(message);
    } else if (message.startsWith("REMOVE:")) {
      // Format: "REMOVE:01:02:03:04"
      processAdminCommand(message);
    } else if (message == "LIST") {
      listAllCards();
    } else if (message == "CLEAR") {
      cardDB.clearAllCards();
      client.publish(admin_topic, "ALL_CARDS_CLEARED");
    }
  }
}

void processAdminCommand(String command) {
  String uidStr = command.substring(command.indexOf(':') + 1);
  byte uid[4];
  
  // Parse UID string to bytes
  for (int i = 0; i < 4; i++) {
    String byteStr = uidStr.substring(i*3, i*3+2);
    uid[i] = (byte)strtol(byteStr.c_str(), NULL, 16);
    
    // Skip the colon
    if (i < 3 && uidStr.charAt(i*3+2) == ':') {
      continue;
    } else {
      break;
    }
  }
  
  if (command.startsWith("ADD:")) {
    uint8_t result = cardDB.addCard(uid);
    if (result == CARD_ADDED) {
      client.publish(admin_topic, "CARD_ADDED");
    } else if (result == CARD_EXISTS) {
      client.publish(admin_topic, "CARD_ALREADY_EXISTS");
    } else if (result == DATABASE_FULL) {
      client.publish(admin_topic, "DATABASE_FULL");
    }
  } else if (command.startsWith("REMOVE:")) {
    uint8_t result = cardDB.removeCard(uid);
    if (result == CARD_REMOVED) {
      client.publish(admin_topic, "CARD_REMOVED");
    } else {
      client.publish(admin_topic, "CARD_NOT_FOUND");
    }
  }
}

void listAllCards() {
  uint8_t cardCount = cardDB.getCardCount();
  
  char countMsg[20];
  sprintf(countMsg, "CARD_COUNT:%d", cardCount);
  client.publish(admin_topic, countMsg);
  
  for (uint8_t i = 0; i < cardCount; i++) {
    byte uid[4];
    if (cardDB.getCardByIndex(i, uid)) {
      char uidStr[15];
      sprintf(uidStr, "CARD:%02X:%02X:%02X:%02X", uid[0], uid[1], uid[2], uid[3]);
      client.publish(admin_topic, uidStr);
      delay(100); // Small delay to avoid overwhelming the MQTT broker
    }
  }
  
  client.publish(admin_topic, "END_OF_LIST");
}

void reconnect() {
  int retries = 0;
  while (!client.connected() && retries < 5) {
    Serial.print(F("Attempting MQTT connection..."));
    printToLCD(F("Connecting MQTT"), F(""));
    
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    // Add more debugging for connection issues
    Serial.println(F("MQTT Server: "));
    Serial.println(mqtt_server);
    Serial.print(F("Port: "));
    Serial.println(mqtt_port);
    
    // Try to connect with a longer timeout and proper error handling
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println(F("connected"));
      client.subscribe(door_control_topic);
      client.subscribe(admin_topic);
      
      printToLCD(F("System Ready"), F(""));
    } else {
      int state = client.state();
      Serial.print(F("failed, rc="));
      Serial.print(state);
      Serial.print(F(" ("));
      
      // Provide more detailed error messages
      switch(state) {
        case -4:
          Serial.print(F("Connection timeout"));
          break;
        case -3:
          Serial.print(F("Connection lost"));
          break;
        case -2:
          Serial.print(F("Connection failed - network issue"));
          break;
        case -1:
          Serial.print(F("Connection refused - bad protocol"));
          break;
        case 1:
          Serial.print(F("Connection refused - bad credentials"));
          break;
        case 2:
          Serial.print(F("Connection refused - not authorized"));
          break;
        case 3:
          Serial.print(F("Connection refused - server unavailable"));
          break;
        case 4:
          Serial.print(F("Connection refused - bad client ID"));
          break;
        case 5:
          Serial.print(F("Connection refused - not authorized"));
          break;
        default:
          Serial.print(F("Unknown error"));
          break;
      }
      
      Serial.println(F(") try again in 5 seconds"));
      
      printToLCD(F("MQTT Failed"), "Error: " + String(state));
      
      // Wait longer between retries to allow network to stabilize
      delay(5000);
      retries++;
    }
  }
  
  if (!client.connected()) {
    printToLCD(F("OFFLINE MODE"), F("No MQTT"));
    Serial.println(F("Running in offline mode - MQTT connection failed"));
    delay(2000);
    
    printToLCD(F("RFID Access"), F("System Ready"));
  }
}

// Function to convert UID bytes to a string
String getUIDString(byte* uid, byte uidSize) {
  String uidString = "";
  for (byte i = 0; i < uidSize; i++) {
    if (uid[i] < 0x10) {
      uidString += "0";
    }
    uidString += String(uid[i], HEX);
    if (i < uidSize - 1) {
      uidString += ":";
    }
  }
  uidString.toUpperCase();
  return uidString;
}

// Function to mask RFID ID for privacy on LCD
String maskRFIDForDisplay(String rfidTag) {
  // If the RFID tag is in format XX:XX:XX:XX, only show the last part
  if (rfidTag.length() >= 11 && rfidTag.indexOf(':') != -1) {
    return "ID: **:**:**:" + rfidTag.substring(rfidTag.lastIndexOf(':') + 1);
  } 
  // For other formats, just show the last 2-4 characters
  else if (rfidTag.length() > 4) {
    return "ID: ****" + rfidTag.substring(rfidTag.length() - 4);
  }
  // If it's not an RFID tag but another identifier
  else {
    return rfidTag;
  }
}

// Function to open the door by moving the servo to the unlock position
void openDoor() {
  Serial.println(F("Executing openDoor() function...")); // Log entry
  // Ensure servo is attached (should be done in setup, but double-check)
  if (!doorServo.attached()) {
      Serial.println(F("Warning: Servo was not attached. Attaching now."));
      doorServo.attach(SERVO_PIN);
  }

  // Unlock position (Assuming 0 is unlock)
  doorServo.write(0);
  Serial.println(F("Door Unlocked (Servo position 0)"));

  doorOpen = true;
  doorOpenTime = millis(); // Start timer for auto-lock
}

// Function to handle access granting
void grantAccess(String identifier) {
  Serial.print(F("Access granted to: "));
  Serial.println(identifier);
  
  // Clear any unauthorized access warning
  unauthorizedAccessDetected = false;
  digitalWrite(RED_LED_PIN, LOW);
  
  // Use masked ID for LCD display
  String displayID = maskRFIDForDisplay(identifier);
  
  // Update LCD
  printToLCD(F("Access Granted"), displayID);
  
  // Visual and audible feedback
  digitalWrite(GREEN_LED_PIN, HIGH);
  digitalWrite(RED_LED_PIN, LOW);
  
  // Sound pattern for access granted
  tone(BUZZER_PIN, 2000, 100);
  delay(200);
  tone(BUZZER_PIN, 2500, 100);
  
  // Open the door - calls the cleaned-up function
  openDoor(); 
  
  // Log to MQTT if connected
  if (client.connected()) {
    // Format: GRANTED:XX:XX:XX:XX
    String accessLog = "GRANTED:" + identifier;
    client.publish(access_log_topic, accessLog.c_str());
    Serial.print(F("Published to MQTT: "));
    Serial.println(accessLog);
  } else {
    Serial.println(F("MQTT not connected, could not publish log"));
  }
  
  // Start monitoring for entry (did they actually go in?)
  startEntryMonitoring(identifier);
  
  // Remove delays here, let the main loop handle timeout and display reset
}

// Function to handle access denial
void denyAccess(String identifier) {
  Serial.print(F("Access denied to: "));
  Serial.println(identifier);
  
  // Ensure the door remains locked (or relock if somehow open)
  // Although openDoor() should not have been called, add safeguard:
  if (doorServo.attached()) {
    doorServo.write(90); // Ensure servo is at Lock position (90)
  }
  doorOpen = false; // Ensure door state is marked as closed/locked

  // Use masked ID for LCD display
  String displayID = maskRFIDForDisplay(identifier);
  
  // Update LCD
  printToLCD(F("Access Denied"), displayID);
  
  // Visual and audible alert
  digitalWrite(RED_LED_PIN, HIGH);
  digitalWrite(GREEN_LED_PIN, LOW);
  
  // Sound pattern for access denied - more noticeable
  tone(BUZZER_PIN, 200, 500);
  delay(600);
  tone(BUZZER_PIN, 150, 500);
  
  // Log to MQTT if connected
  if (client.connected()) {
    // Format: DENIED:XX:XX:XX:XX
    String accessLog = "DENIED:" + identifier;
    client.publish(access_log_topic, accessLog.c_str());
    Serial.print(F("Published to MQTT: "));
    Serial.println(accessLog);
  } else {
    Serial.println(F("MQTT not connected, could not publish log"));
  }
  
  // Keep the red LED on solid for initial warning - remove delay
  // delay(3000); // Remove this delay to allow loop to continue
  
  // Start the continuous blinking pattern for sustained warning
  unauthorizedAccessDetected = true;
  lastRedLEDBlinkTime = millis();
  unauthorizedWarningStartTime = millis();
  
  // Reset display after a short time (allow user to see denial)
  // This will be overwritten if another card is scanned quickly,
  // or eventually reset by the loop's general state check if door was open.
  // Consider adding a timed reset for the denial message if needed.
  // For now, rely on blinking LED and next scan/timeout to reset LCD.
  // printToLCD(F("RFID Access"), F("System Ready")); // Maybe delay this reset
}

// Function to handle admin mode
void enterAdminMode() {
  adminMode = true;
  adminModeStartTime = millis();
  
  // Clear any unauthorized access warning
  unauthorizedAccessDetected = false;
  digitalWrite(RED_LED_PIN, LOW);
  
  printToLCD(F("ADMIN MODE"), F("Scan card..."));
  
  // Notification beep
  digitalWrite(BUZZER_PIN, HIGH);
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);
  delay(200);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);
  
  client.publish(admin_topic, "ADMIN_MODE_ACTIVATED");
}

void exitAdminMode() {
  adminMode = false;
  cardWasScanned = false;
  
  printToLCD(F("RFID Access"), F("System Ready"));
  
  client.publish(admin_topic, "ADMIN_MODE_DEACTIVATED");
}

// Function to handle card in admin mode
void processAdminCard(byte* uid, byte uidSize) {
  String uidString = getUIDString(uid, uidSize);
  String displayID = maskRFIDForDisplay(uidString);
  
  if (!cardWasScanned) {
    // First card scan in admin mode - store card
    memcpy(lastScannedCard, uid, 4);
    cardWasScanned = true;
    
    printToLCD(F("Add/Remove Card?"), displayID);
    
    // Extend admin mode timeout
    adminModeStartTime = millis();
  } else {
    // Second card scan - check if it's the same card
    bool sameCard = true;
    for (byte i = 0; i < 4; i++) {
      if (lastScannedCard[i] != uid[i]) {
        sameCard = false;
        break;
      }
    }
    
    if (sameCard) {
      // Same card - toggle its status
      if (cardDB.isCardAuthorized(uid)) {
        // Card exists - remove it
        uint8_t result = cardDB.removeCard(uid);
        
        printToLCD(F("Card Removed"), displayID);
        
        client.publish(admin_topic, ("CARD_REMOVED:" + uidString).c_str());
      } else {
        // Card doesn't exist - add it
        uint8_t result = cardDB.addCard(uid);
        
        if (result == CARD_ADDED) {
          printToLCD(F("Card Added"), displayID);
          
          client.publish(admin_topic, ("CARD_ADDED:" + uidString).c_str());
        } else if (result == DATABASE_FULL) {
          printToLCD(F("Database Full!"), F(""));
          
          client.publish(admin_topic, "DATABASE_FULL");
        }
      }
      
      // Reset for next card
      cardWasScanned = false;
      delay(2000);
      
      // Exit admin mode
      exitAdminMode();
    } else {
      // Different card - start over
      memcpy(lastScannedCard, uid, 4);
      
      printToLCD(F("Add/Remove Card?"), displayID);
      
      // Extend admin mode timeout
      adminModeStartTime = millis();
    }
  }
}

// Function to initialize ultrasonic sensor
void setupUltrasonicSensor() {
  pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_ECHO_PIN, INPUT);
  Serial.println(F("Ultrasonic sensor initialized"));
  
  // Take initial measurement
  lastDistance = readUltrasonicDistance();
  Serial.print(F("Initial distance: "));
  Serial.print(lastDistance);
  Serial.println(F(" cm"));
}

// Function to read distance from ultrasonic sensor
float readUltrasonicDistance() {
  // Clear the trigger pin
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  // Set the trigger pin HIGH for 10 microseconds
  digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
  
  // Read the echo pin - returns sound wave travel time in microseconds
  long duration = pulseIn(ULTRASONIC_ECHO_PIN, HIGH, 30000); // 30ms timeout
  
  // Calculate distance in centimeters
  // Speed of sound = 340 m/s = 0.034 cm/µs
  float distance = duration * 0.034 / 2;
  
  // Handle timeout (no echo received)
  if (duration == 0) {
    distance = -1; // Error indicator
  }
  
  return distance;
}

// Function to check if a person is in front of the sensor
bool isPersonPresent() {
  float distance = readUltrasonicDistance();
  
  if (distance > 0 && distance < PERSON_DISTANCE_THRESHOLD) {
    return true;
  }
  return false;
}

// Function to start monitoring for entry after access granted
void startEntryMonitoring(String identifier) {
  monitoringEntry = true;
  personEntered = false;
  entryMonitorStartTime = millis();
  Serial.println(F("Started monitoring for entry"));
  
  // Take initial reading
  lastDistance = readUltrasonicDistance();
  
  // Reset detection variables
  minDistance = 1000;
  maxDistance = 0;
  inDetectionState = false;
  lastDetectionTime = 0;
}

// Function to check if person entered after access granted
void checkPersonEntered(String identifier) {
  if (!monitoringEntry) return;
  
  // Get current distance reading
  float currentDistance = readUltrasonicDistance();
  
  // Skip invalid readings
  if (currentDistance <= 0) return;
  
  // Update min/max distances for better detection
  if (currentDistance < minDistance) minDistance = currentDistance;
  if (currentDistance > maxDistance) maxDistance = currentDistance;
  
  // Only consider significant distance changes
  float distanceChange = abs(currentDistance - lastDistance);
  unsigned long timeSinceLastDetection = millis() - lastDetectionTime;
  
  // Check for a person passing through the doorway with improved conditions
  if (distanceChange > DISTANCE_CHANGE_THRESHOLD && 
      timeSinceLastDetection > MIN_DETECTION_INTERVAL &&
      (maxDistance - minDistance) > DISTANCE_CHANGE_THRESHOLD) {
      
    // Toggle detection state
    inDetectionState = !inDetectionState;
    
    // Only count when leaving detection state
    if (!inDetectionState) {
      personEntered = true;
      lastDetectionTime = millis();
      Serial.println(F("Person entered"));
      
      // Start monitoring for tailgating
      monitoringEntry = false;
      startTailgatingMonitoring();
    }
  }
  
  // Check if monitoring timeout elapsed
  if (millis() - entryMonitorStartTime > DOOR_MONITOR_TIMEOUT) {
    monitoringEntry = false;
    
    // If person did not enter, send security alert
    if (!personEntered) {
      Serial.println(F("SECURITY ALERT: Access granted but no entry detected"));
      
      // Publish security event to MQTT
      publishSecurityEvent(
        EVENT_TYPE_NO_ENTRY, 
        "Card used but no entry detected", 
        SEVERITY_MEDIUM
      );
      
      // Visual/audio alert
      digitalWrite(RED_LED_PIN, HIGH);
      tone(BUZZER_PIN, 400, 1000);
      delay(1500);
      digitalWrite(RED_LED_PIN, LOW);
    }
    
    // Reset detection variables
    inDetectionState = false;
    minDistance = 1000;
    maxDistance = 0;
  }
  
  // Update last distance
  lastDistance = currentDistance;
}

// Function to start monitoring for tailgating
void startTailgatingMonitoring() {
  monitoringTailgating = true;
  personCount = 1; // First person already entered
  entryMonitorStartTime = millis(); // Reuse the same variable for simplicity
  Serial.println(F("Started monitoring for tailgating"));
  
  // Reset detection variables for tailgating
  minDistance = 1000;
  maxDistance = 0;
  inDetectionState = false;
  lastDetectionTime = millis(); // Set last detection time to now (person just entered)
}

// Function to enter ultrasonic sensor test mode
void enterUltrasonicTestMode() {
  ultrasonicTestMode = true;
  Serial.println(F("===== ULTRASONIC SENSOR TEST MODE ====="));
  Serial.println(F("Press the ADMIN button to exit test mode"));
  Serial.println(F("Continuous distance readings will be shown"));
  Serial.println(F("Recommended steps:"));
  Serial.println(F("1. Place sensor at the side of doorway"));
  Serial.println(F("2. Walk through the door to see readings"));
  Serial.println(F("3. Adjust PERSON_DISTANCE_THRESHOLD if needed"));
  Serial.println(F("======================================"));
  
  printToLCD(F("SENSOR TEST"), F("MODE ACTIVE"));
  
  // Use LED to indicate test mode
  digitalWrite(GREEN_LED_PIN, HIGH);
}

// Function to exit ultrasonic sensor test mode
void exitUltrasonicTestMode() {
  ultrasonicTestMode = false;
  Serial.println(F("Exiting ultrasonic test mode"));
  
  // Reset LEDs
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  
  printToLCD(F("RFID Access"), F("System Ready"));
}

// Function for running the ultrasonic sensor test
void runUltrasonicTest() {
  static unsigned long lastTestPrint = 0;
  static bool personWasPresent = false;
  static float minTestDistance = 1000;
  static float maxTestDistance = 0;
  
  // Read current distance
  float distance = readUltrasonicDistance();
  
  // Track min/max distances to help with calibration
  if (distance > 0) {  // Valid reading
    if (distance < minTestDistance) minTestDistance = distance;
    if (distance > maxTestDistance) maxTestDistance = distance;
  }
  
  // Determine if person is detected based on threshold
  bool personPresent = (distance > 0 && distance < PERSON_DISTANCE_THRESHOLD);
  
  // Print distance reading every 500ms
  if (millis() - lastTestPrint > 500) {
    Serial.print(F("Distance: "));
    Serial.print(distance);
    Serial.print(F(" cm | Person: "));
    Serial.print(personPresent ? F("YES") : F("NO"));
    Serial.print(F(" | Min: "));
    Serial.print(minTestDistance);
    Serial.print(F(" | Max: "));
    Serial.println(maxTestDistance);
    
    // Update LCD with reading
    char distanceStr[16];
    sprintf(distanceStr, "D:%d Min:%d Max:%d", (int)distance, (int)minTestDistance, (int)maxTestDistance);
    printToLCD(F("SENSOR TEST"), distanceStr);
    
    lastTestPrint = millis();
  }
  
  // Use LEDs to indicate person detection
  digitalWrite(RED_LED_PIN, personPresent);
  
  // Detect state change (person passing through)
  if (personPresent != personWasPresent) {
    if (personPresent) {
      Serial.println(F("TEST: Person ENTERED detection zone"));
      tone(BUZZER_PIN, 2000, 100);
    } else {
      Serial.println(F("TEST: Person LEFT detection zone"));
      tone(BUZZER_PIN, 1000, 100);
      
      // Reset min/max on exit for next pass test
      minTestDistance = 1000;
      maxTestDistance = 0;
    }
    personWasPresent = personPresent;
  }
  
  // Check if user wants to exit test mode (press admin button)
  if (digitalRead(ADMIN_BUTTON) == LOW) {
    delay(50); // Debounce
    if (digitalRead(ADMIN_BUTTON) == LOW) {
      while (digitalRead(ADMIN_BUTTON) == LOW) {
        delay(10); // Wait for button release
      }
      exitUltrasonicTestMode();
    }
  }
}

// Function to check for tailgating (multiple people entering)
void checkTailgating() {
  if (!monitoringTailgating) return;
  
  // Get current distance reading
  float currentDistance = readUltrasonicDistance();
  
  // Skip invalid readings
  if (currentDistance <= 0) return;
  
  // Update min/max distances for better detection
  if (currentDistance < minDistance) minDistance = currentDistance;
  if (currentDistance > maxDistance) maxDistance = currentDistance;
  
  // Only consider significant distance changes
  float distanceChange = abs(currentDistance - lastDistance);
  unsigned long timeSinceLastDetection = millis() - lastDetectionTime;
  
  // Check for a person passing through the doorway with improved conditions:
  // 1. Significant distance change
  // 2. Enough time since last detection
  // 3. Range difference indicates a person actually moved through
  if (distanceChange > DISTANCE_CHANGE_THRESHOLD && 
      timeSinceLastDetection > MIN_DETECTION_INTERVAL &&
      (maxDistance - minDistance) > DISTANCE_CHANGE_THRESHOLD) {
      
    // Toggle detection state
    inDetectionState = !inDetectionState;
    
    // Only count when leaving detection state
    if (!inDetectionState) {
      personCount++;
      lastDetectionTime = millis();
      
      // Reset min/max for next detection
      minDistance = 1000;
      maxDistance = 0;
      
      Serial.print(F("Person count: "));
      Serial.println(personCount);
      
      if (personCount > 1) {
        // Tailgating detected!
        Serial.println(F("SECURITY ALERT: Tailgating detected"));
        
        // Publish security event to MQTT
        publishSecurityEvent(
          EVENT_TYPE_TAILGATING, 
          "Multiple people entered with single card", 
          SEVERITY_HIGH
        );
        
        // Sound alarm
        digitalWrite(RED_LED_PIN, HIGH);
        tone(BUZZER_PIN, 800, 500);
        delay(300);
        tone(BUZZER_PIN, 800, 500);
        delay(300);
        tone(BUZZER_PIN, 800, 1000);
        digitalWrite(RED_LED_PIN, LOW);
        
        monitoringTailgating = false;
      }
    }
  }
  
  // Check if monitoring timeout elapsed
  if (millis() - entryMonitorStartTime > TAILGATING_TIMEOUT) {
    monitoringTailgating = false;
    Serial.println(F("Tailgating monitoring ended"));
    
    // Reset detection variables
    inDetectionState = false;
    minDistance = 1000;
    maxDistance = 0;
  }
  
  // Update last distance
  lastDistance = currentDistance;
}

// Function to publish security event to MQTT
void publishSecurityEvent(const char* eventType, const char* description, const char* severity) {
  if (!client.connected()) return;
  
  // Create JSON-like string format for security events
  // Format: {"sensor_id":1,"event_type":"tailgating","description":"Multiple people entered with single card","severity":"high","status":"active","duration":0}
  
  String securityEvent = "{\"sensor_id\":";
  securityEvent += ULTRASONIC_SENSOR_ID;
  securityEvent += ",\"event_type\":\"";
  securityEvent += eventType;
  securityEvent += "\",\"description\":\"";
  securityEvent += description;
  securityEvent += "\",\"severity\":\"";
  securityEvent += severity;
  securityEvent += "\",\"status\":\"active\",\"duration\":0}";
  
  // Publish to security topic
  client.publish(security_topic, securityEvent.c_str());
  Serial.print(F("Published security event: "));
  Serial.println(securityEvent);
}

void loop() {
  // Special test mode for ultrasonic calibration
  if (ultrasonicTestMode) {
    runUltrasonicTest();
    return; // Skip the rest of the loop in test mode
  }
  
  // Check WiFi first, reconnect if needed
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("WiFi connection lost, reconnecting..."));
    // Attempt to reconnect WiFi without blocking everything indefinitely
    // setup_wifi(); // This blocks, consider a non-blocking approach if needed
    WiFi.begin(ssid, password); // Try reconnecting
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) { // 10 sec timeout
        delay(500);
        Serial.print(F("."));
    }
    if(WiFi.status() != WL_CONNECTED) {
        Serial.println(F(" WiFi reconnect failed."));
    } else {
        Serial.println(F(" WiFi reconnected."));
    }
  }
  
  // Check MQTT connection if WiFi is connected
  if (WiFi.status() == WL_CONNECTED && !client.connected()) {
    reconnect(); // reconnect() already has retry logic
  }
  
  // Only call client.loop() if connected to avoid blocking
  if (client.connected()) {
    client.loop();
  }
  
  // Handle unauthorized access warning (blinking red LED)
  if (unauthorizedAccessDetected) {
    // Blink the LED at the set interval
    if (millis() - lastRedLEDBlinkTime > RED_LED_BLINK_INTERVAL) {
      // Toggle the red LED
      digitalWrite(RED_LED_PIN, !digitalRead(RED_LED_PIN));
      lastRedLEDBlinkTime = millis();
    }
    
    // Reset unauthorized warning after the duration
    if (millis() - unauthorizedWarningStartTime > UNAUTHORIZED_WARNING_DURATION) {
      unauthorizedAccessDetected = false;
      digitalWrite(RED_LED_PIN, LOW); // Ensure LED is off after warning period
      Serial.println(F("Unauthorized access warning period ended."));
       // Optionally reset LCD here if it wasn't reset by other actions
       if (!doorOpen) { // Only reset if door isn't meant to be open
           printToLCD(F("RFID Access"), F("System Ready"));
       }
    }
  }
  
  // Check admin button
  if (digitalRead(ADMIN_BUTTON) == LOW && !adminMode) {
    delay(50); // Debounce
    if (digitalRead(ADMIN_BUTTON) == LOW) {
      Serial.println(F("Admin button pressed. Entering admin mode."));
      enterAdminMode();
      while (digitalRead(ADMIN_BUTTON) == LOW) {
        delay(10); // Wait for button release
      }
    }
  }
  
  // Check admin mode timeout
  if (adminMode && (millis() - adminModeStartTime > ADMIN_MODE_TIMEOUT)) {
    Serial.println(F("Admin mode timed out. Exiting admin mode."));
    exitAdminMode();
  }
  
  // Check if the door should be closed (auto-lock)
  if (doorOpen && (millis() - doorOpenTime > DOOR_OPEN_DURATION)) {
    // Lock the door explicitly
    if (doorServo.attached()) {
        doorServo.write(90); // Lock position (Assuming 90 is lock)
        Serial.println(F("Door Automatically Locked (Servo position 90)"));
    } else {
        Serial.println(F("Warning: Servo not attached during auto-lock attempt."));
    }
    digitalWrite(GREEN_LED_PIN, LOW); // Turn off green LED
    doorOpen = false;                 // Update door state
    printToLCD(F("RFID Access"), F("System Ready")); // Update LCD
    // Clear any entry/tailgating monitoring state if needed
    monitoringEntry = false;
    monitoringTailgating = false;
     Serial.println(F("Door state reset to closed/locked."));
  }
  
  // Check ultrasonic sensor for security monitoring
  if (monitoringEntry) {
    checkPersonEntered(lastScannedCardUID);
  }
  
  if (monitoringTailgating) {
    checkTailgating();
  }
  
  // Look for new RFID cards
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return; // No card, continue loop
  }
  
  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    mfrc522.PICC_HaltA(); // Halt card on read error
    mfrc522.PCD_StopCrypto1();
    return; // Error reading card, continue loop
  }
  
  // Show UID on serial monitor
  String uidString = getUIDString(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.print(F("Card Scan Detected. UID: "));
  Serial.println(uidString);
  
  // Save the card UID for monitoring (if needed for security features)
  lastScannedCardUID = uidString;
  
  // Handle card based on current mode
  if (adminMode) {
    Serial.println(F("Admin mode active. Processing card for admin functions."));
    processAdminCard(mfrc522.uid.uidByte, mfrc522.uid.size);
  } else {
    // Check if the scanned card is authorized
    Serial.print(F("Checking authorization for UID: "));
    Serial.println(uidString);
    bool authorized = cardDB.isCardAuthorized(mfrc522.uid.uidByte);
    Serial.print(F("Authorization result: "));
    Serial.println(authorized ? "GRANTED" : "DENIED");

    if (authorized) {
      Serial.println(F("Calling grantAccess()..."));
      grantAccess(uidString); // Calls openDoor()
    } else {
      Serial.println(F("Calling denyAccess()..."));
      denyAccess(uidString); // Does NOT call openDoor(), ensures door is locked
    }
  }
  
  // Halt PICC & Stop Crypto
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  // Add a small delay to prevent rapid re-scans if card is held near reader
  delay(500); 
} 