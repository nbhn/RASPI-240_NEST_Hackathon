#ifndef ACCESS_DATABASE_H
#define ACCESS_DATABASE_H

#include <Arduino.h>
#include <EEPROM.h>

// EEPROM Map
#define EEPROM_SIZE 1024
#define EEPROM_INIT_ADDR 0      // 1 byte to check if EEPROM is initialized
#define EEPROM_COUNT_ADDR 1     // 1 byte to store the count of saved cards
#define EEPROM_CARDS_START 2    // Where card UIDs storage starts
#define MAX_CARDS 20            // Maximum number of cards that can be stored
#define UID_SIZE 4              // Size of RFID UID in bytes

// Card status codes
#define CARD_ADDED 1
#define CARD_REMOVED 2
#define CARD_EXISTS 3
#define DATABASE_FULL 4
#define DATABASE_ERROR 5

class AccessDatabase {
  private:
    bool initialized = false;
    
    // Initialize EEPROM with default values if not already initialized
    void initializeEEPROM() {
      if (EEPROM.read(EEPROM_INIT_ADDR) != 0xAA) {
        // Set initialization flag
        EEPROM.write(EEPROM_INIT_ADDR, 0xAA);
        // Set card count to 0
        EEPROM.write(EEPROM_COUNT_ADDR, 0);
        EEPROM.commit();
      }
      initialized = true;
    }
    
  public:
    // Initialize database
    void begin() {
      EEPROM.begin(EEPROM_SIZE);
      initializeEEPROM();
    }
    
    // Get number of stored cards
    uint8_t getCardCount() {
      if (!initialized) initializeEEPROM();
      return EEPROM.read(EEPROM_COUNT_ADDR);
    }
    
    // Check if a card UID exists in the database
    bool isCardAuthorized(byte* uid) {
      if (!initialized) initializeEEPROM();
      
      uint8_t cardCount = getCardCount();
      for (uint8_t i = 0; i < cardCount; i++) {
        bool match = true;
        for (uint8_t j = 0; j < UID_SIZE; j++) {
          if (EEPROM.read(EEPROM_CARDS_START + (i * UID_SIZE) + j) != uid[j]) {
            match = false;
            break;
          }
        }
        if (match) return true;
      }
      return false;
    }
    
    // Add a new card to the database
    uint8_t addCard(byte* uid) {
      if (!initialized) initializeEEPROM();
      
      // Check if card already exists
      if (isCardAuthorized(uid)) {
        return CARD_EXISTS;
      }
      
      uint8_t cardCount = getCardCount();
      
      // Check if database is full
      if (cardCount >= MAX_CARDS) {
        return DATABASE_FULL;
      }
      
      // Add the new card
      for (uint8_t j = 0; j < UID_SIZE; j++) {
        EEPROM.write(EEPROM_CARDS_START + (cardCount * UID_SIZE) + j, uid[j]);
      }
      
      // Increment card count
      EEPROM.write(EEPROM_COUNT_ADDR, cardCount + 1);
      EEPROM.commit();
      
      return CARD_ADDED;
    }
    
    // Remove a card from the database
    uint8_t removeCard(byte* uid) {
      if (!initialized) initializeEEPROM();
      
      uint8_t cardCount = getCardCount();
      
      // Find the card
      int cardIndex = -1;
      for (uint8_t i = 0; i < cardCount; i++) {
        bool match = true;
        for (uint8_t j = 0; j < UID_SIZE; j++) {
          if (EEPROM.read(EEPROM_CARDS_START + (i * UID_SIZE) + j) != uid[j]) {
            match = false;
            break;
          }
        }
        if (match) {
          cardIndex = i;
          break;
        }
      }
      
      // If card not found
      if (cardIndex == -1) {
        return DATABASE_ERROR;
      }
      
      // Move all cards after this one up one slot
      for (int i = cardIndex; i < cardCount - 1; i++) {
        for (uint8_t j = 0; j < UID_SIZE; j++) {
          byte value = EEPROM.read(EEPROM_CARDS_START + ((i + 1) * UID_SIZE) + j);
          EEPROM.write(EEPROM_CARDS_START + (i * UID_SIZE) + j, value);
        }
      }
      
      // Decrement card count
      EEPROM.write(EEPROM_COUNT_ADDR, cardCount - 1);
      EEPROM.commit();
      
      return CARD_REMOVED;
    }
    
    // Get a specific card UID by index
    bool getCardByIndex(uint8_t index, byte* uid) {
      if (!initialized) initializeEEPROM();
      
      uint8_t cardCount = getCardCount();
      
      if (index >= cardCount) {
        return false;
      }
      
      for (uint8_t j = 0; j < UID_SIZE; j++) {
        uid[j] = EEPROM.read(EEPROM_CARDS_START + (index * UID_SIZE) + j);
      }
      
      return true;
    }
    
    // Clear all cards from the database
    void clearAllCards() {
      if (!initialized) initializeEEPROM();
      
      EEPROM.write(EEPROM_COUNT_ADDR, 0);
      EEPROM.commit();
    }
};

#endif 