#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

// Define the display. For a 0.96" OLED, it's typically 128x64 pixels with I2C connection
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Matrix animation configuration
const uint8_t CHAR_WIDTH = 6;        // Width of each character in pixels
const uint8_t CHAR_HEIGHT = 8;       // Height of each character 
const uint8_t NUM_COLS = 21;         // 128/6 = 21.33, round down to 21
const uint8_t NUM_ROWS = 8;          // 64/8 = 8

// Display settings
const uint8_t DISPLAY_CONTRAST = 255;  // Maximum brightness (0-255)

// Precomputed character set
const char MATRIX_CHARS[] = "01234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()_+-=[]";
const uint8_t CHAR_SET_SIZE = sizeof(MATRIX_CHARS) - 1;

// Structure for each column
struct MatrixColumn {
  int8_t pos;              // Current Y position of leading character
  uint8_t speed;           // How fast this column falls
  uint8_t length;          // Length of the trail
  uint32_t lastUpdate;     // Time of last position update
  uint32_t lastCharChange; // Time of last character change
  bool active;             // Whether this column is currently active
  char chars[12];          // Characters for each position (larger than NUM_ROWS for safety)
};

MatrixColumn columns[NUM_COLS];

// Time variables
uint32_t lastDropTime = 0;
const uint16_t DROP_DELAY = 100;        // Time between new drops
const uint16_t CHAR_CHANGE_DELAY = 500; // Slower character change (500ms)
const uint8_t MIN_SPEED = 3;            // Minimum speed
const uint8_t MAX_SPEED = 6;            // Maximum speed

void setup() {
  // Initialize the display
  Wire.setClock(400000);  // Set I2C to 400kHz for faster communication
  u8g2.begin();
  
  // Set maximum contrast/brightness
  u8g2.setContrast(DISPLAY_CONTRAST);
  
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.setFontMode(1);
  
  randomSeed(analogRead(0));
  
  // Initialize all columns
  for (uint8_t i = 0; i < NUM_COLS; i++) {
    columns[i].active = false;
    columns[i].pos = -1;
    columns[i].speed = random(MIN_SPEED, MAX_SPEED + 1);
    columns[i].length = random(3, 8);
    columns[i].lastUpdate = 0;
    columns[i].lastCharChange = 0;
    
    // Initialize with random characters
    for (uint8_t j = 0; j < 12; j++) {
      columns[i].chars[j] = MATRIX_CHARS[random(CHAR_SET_SIZE)];
    }
  }
}

// Get a new random matrix character
char getRandomChar() {
  return MATRIX_CHARS[random(CHAR_SET_SIZE)];
}

void loop() {
  uint32_t currentTime = millis();
  
  // Create new drops
  if (currentTime - lastDropTime > DROP_DELAY) {
    // Try to activate new columns
    for (uint8_t attempt = 0; attempt < 2; attempt++) {
      uint8_t col = random(NUM_COLS);
      if (!columns[col].active) {
        columns[col].active = true;
        columns[col].pos = -1 * (int8_t)(columns[col].length); // Start above screen
        columns[col].speed = random(MIN_SPEED, MAX_SPEED + 1);
        columns[col].length = random(3, 8);
        columns[col].lastUpdate = currentTime;
        columns[col].lastCharChange = currentTime;
        
        // Reset characters - FIXED: was using i instead of col
        for (uint8_t j = 0; j < 12; j++) {
          columns[col].chars[j] = getRandomChar();
        }
      }
    }
    lastDropTime = currentTime;
  }
  
  // Update the display
  u8g2.clearBuffer();
  
  // Process each column
  for (uint8_t i = 0; i < NUM_COLS; i++) {
    if (columns[i].active) {
      // Update position at intervals based on speed
      if (currentTime - columns[i].lastUpdate > (300 / columns[i].speed)) {
        columns[i].pos++;
        
        // If column has reached the bottom, deactivate it
        if (columns[i].pos > NUM_ROWS + columns[i].length) {
          columns[i].active = false;
        }
        
        columns[i].lastUpdate = currentTime;
      }
      
      // Change head character periodically
      if (currentTime - columns[i].lastCharChange > CHAR_CHANGE_DELAY) {
        // Get the position of the leading character
        if (columns[i].pos >= 0 && columns[i].pos < 12) {
          columns[i].chars[columns[i].pos] = getRandomChar();
        }
        columns[i].lastCharChange = currentTime;
      }
      
      // Draw the column with fading trail
      for (int8_t j = 0; j < columns[i].length; j++) {
        int8_t drawPos = columns[i].pos - j;
        
        // Only draw if on screen
        if (drawPos >= 0 && drawPos < NUM_ROWS) {
          uint8_t yPos = drawPos * CHAR_HEIGHT;
          uint8_t xPos = i * CHAR_WIDTH;
          
          // Set drawing color (1 = pixel on, 0 = pixel off)
          u8g2.setDrawColor(1);
          
          // Get character from stored array, with bounds check
          char c = (drawPos < 12) ? columns[i].chars[drawPos] : '#';
          
          // Draw the character
          u8g2.setCursor(xPos, yPos + CHAR_HEIGHT);
          u8g2.print(c);
        }
      }
    }
  }
  
  // Send buffer to display
  u8g2.sendBuffer();
}