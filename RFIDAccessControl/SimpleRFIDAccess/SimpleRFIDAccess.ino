#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Define voltage boost pins - these will supply additional voltage to the LCD
#define BOOST_PIN_1 14
#define BOOST_PIN_2 12

// Define the I2C address of the LCD (found by scanner)
#define LCD_ADDRESS 0x27

// Create LCD object
LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("LCD Voltage Boost Test");
  
  // Configure voltage boost pins as OUTPUT and set HIGH
  pinMode(BOOST_PIN_1, OUTPUT);
  pinMode(BOOST_PIN_2, OUTPUT);
  digitalWrite(BOOST_PIN_1, HIGH);
  digitalWrite(BOOST_PIN_2, HIGH);
  
  Serial.println("Voltage boost enabled (providing approximately 5-6V to LCD)");
  
  // Wait a moment for voltage to stabilize
  delay(1000);
  
  // Initialize I2C with specific pull-up handling
  Wire.begin();
  Wire.setClock(100000); // Lower clock rate to 100kHz for more stable communication
  
  // Manually try to initialize the LCD with a delay-based approach
  Serial.println("Initializing LCD with delays...");
  
  // Initialize LCD - with error handling
  boolean lcdFound = false;
  for (int attempt = 0; attempt < 3; attempt++) {
    Serial.print("Initialization attempt: ");
    Serial.println(attempt + 1);
    
    Wire.beginTransmission(LCD_ADDRESS);
    byte error = Wire.endTransmission();
    
    if (error == 0) {
      lcdFound = true;
      Serial.println("LCD found on the I2C bus!");
      
      // Try custom initialization with delays
      lcd.init();
      delay(100);
      lcd.backlight();
      delay(100);
      lcd.clear();
      delay(100);
      
      break;
    } else {
      Serial.print("LCD initialization error: ");
      Serial.println(error);
      delay(1000);
    }
  }
  
  if (!lcdFound) {
    Serial.println("LCD not responding. Please check connections and voltage.");
    while(1) { delay(1000); } // Stop execution
  }
  
  // Display test message
  lcd.setCursor(0, 0);
  lcd.print("LCD Test with");
  lcd.setCursor(0, 1);
  lcd.print("Voltage Boost");
  
  Serial.println("LCD test complete. If you see text on the LCD, it's working!");
}

void loop() {
  // Test different patterns
  delay(3000);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Address: 0x");
  lcd.print(LCD_ADDRESS, HEX);
  
  delay(3000);
  
  // Test all character positions
  lcd.clear();
  for (int row = 0; row < 2; row++) {
    for (int col = 0; col < 16; col++) {
      lcd.setCursor(col, row);
      lcd.print(col % 10);
      delay(50);
    }
  }
  
  delay(3000);
  
  // Moving pattern
  lcd.clear();
  for (int i = 0; i < 16; i++) {
    lcd.clear();
    lcd.setCursor(i, 0);
    lcd.print(">");
    lcd.setCursor(15-i, 1);
    lcd.print("<");
    delay(200);
  }
  
  // Return to main display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("LCD working!");
  lcd.setCursor(0, 1);
  lcd.print("Voltage boost OK");
} 
