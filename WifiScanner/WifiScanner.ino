//qwen 3.5 plus
#include <WiFi.h>
#include "Display_ST7789.h"

// Simple 5x7 bitmap font (ASCII 32-126)
const uint8_t font5x7[][5] = {
  {0x00,0x00,0x00,0x00,0x00}, {0x00,0x00,0x5F,0x00,0x00},
  {0x00,0x07,0x00,0x07,0x00}, {0x14,0x7F,0x14,0x7F,0x14},
  {0x24,0x2A,0x7F,0x2A,0x12}, {0x23,0x13,0x08,0x64,0x62},
  {0x36,0x49,0x55,0x22,0x50}, {0x00,0x05,0x03,0x00,0x00},
  {0x00,0x1C,0x22,0x41,0x00}, {0x00,0x41,0x22,0x1C,0x00},
  {0x14,0x08,0x3E,0x08,0x14}, {0x08,0x08,0x3E,0x08,0x08},
  {0x00,0x50,0x30,0x00,0x00}, {0x08,0x08,0x08,0x08,0x08},
  {0x00,0x60,0x60,0x00,0x00}, {0x20,0x10,0x08,0x04,0x02},
  {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
  {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
  {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
  {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
  {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E},
  {0x00,0x36,0x36,0x00,0x00}, {0x00,0x56,0x36,0x00,0x00},
  {0x08,0x14,0x22,0x41,0x00}, {0x14,0x14,0x14,0x14,0x14},
  {0x00,0x41,0x22,0x14,0x08}, {0x02,0x01,0x51,0x09,0x06},
  {0x32,0x49,0x79,0x41,0x3E}, {0x7E,0x11,0x11,0x11,0x7E},
  {0x7F,0x49,0x49,0x49,0x36}, {0x3E,0x41,0x41,0x41,0x22},
  {0x7F,0x41,0x41,0x22,0x1C}, {0x7F,0x49,0x49,0x49,0x41},
  {0x7F,0x09,0x09,0x09,0x01}, {0x3E,0x41,0x49,0x49,0x7A},
  {0x7F,0x08,0x08,0x08,0x7F}, {0x00,0x41,0x7F,0x41,0x00},
  {0x20,0x40,0x41,0x3F,0x01}, {0x7F,0x08,0x14,0x22,0x41},
  {0x7F,0x40,0x40,0x40,0x40}, {0x7F,0x02,0x0C,0x02,0x7F},
  {0x7F,0x04,0x08,0x10,0x7F}, {0x3E,0x41,0x41,0x41,0x3E},
  {0x7F,0x09,0x19,0x29,0x46}, {0x46,0x49,0x49,0x49,0x31},
  {0x01,0x01,0x7F,0x01,0x01}, {0x3F,0x40,0x40,0x40,0x3F},
  {0x1F,0x20,0x40,0x20,0x1F}, {0x3F,0x40,0x38,0x40,0x3F},
  {0x63,0x14,0x08,0x14,0x63}, {0x07,0x08,0x70,0x08,0x07},
  {0x61,0x51,0x49,0x45,0x43}, {0x00,0x7F,0x41,0x41,0x00},
  {0x02,0x04,0x08,0x10,0x20}, {0x00,0x41,0x41,0x7F,0x00},
  {0x04,0x02,0x01,0x02,0x04}, {0x40,0x40,0x40,0x40,0x40},
  {0x00,0x01,0x02,0x04,0x00}, {0x20,0x54,0x54,0x54,0x78},
  {0x7F,0x48,0x44,0x44,0x38}, {0x38,0x44,0x44,0x44,0x20},
  {0x38,0x44,0x44,0x48,0x7F}, {0x38,0x54,0x54,0x54,0x18},
  {0x08,0x7E,0x09,0x01,0x02}, {0x08,0x14,0x54,0x54,0x3C},
  {0x7F,0x08,0x04,0x04,0x78}, {0x00,0x44,0x7D,0x40,0x00},
  {0x20,0x40,0x44,0x3D,0x00}, {0x7F,0x10,0x28,0x44,0x00},
  {0x00,0x41,0x7F,0x40,0x00}, {0x7C,0x04,0x18,0x04,0x7C},
  {0x7C,0x08,0x04,0x04,0x78}, {0x38,0x44,0x44,0x44,0x38},
  {0x7C,0x14,0x14,0x14,0x08}, {0x08,0x14,0x14,0x18,0x7C},
  {0x7C,0x08,0x04,0x04,0x08}, {0x48,0x54,0x54,0x54,0x20},
  {0x04,0x3F,0x44,0x40,0x20}, {0x3C,0x40,0x40,0x20,0x7C},
  {0x0C,0x50,0x50,0x50,0x3C}, {0x44,0x28,0x10,0x28,0x44},
  {0x00,0x08,0x36,0x41,0x00}, {0x00,0x00,0x7F,0x00,0x00},
  {0x00,0x41,0x36,0x08,0x00}, {0x10,0x08,0x08,0x10,0x08},
  {0x78,0x46,0x41,0x46,0x78}
};

// Color definitions (RGB565)
#define BLACK       0x0000
#define WHITE       0xFFFF
#define GREEN       0x07E0
#define YELLOW      0xFFE0
#define RED         0xF800
#define BLUE        0x001F
#define GRAY        0x8410

// Display dimensions
#define DISPLAY_WIDTH   172
#define DISPLAY_HEIGHT  320
#define FONT_WIDTH      6
#define FONT_HEIGHT     8
#define LINE_HEIGHT     12
#define MAX_APS         18

// WiFi scan data structure
struct APInfo {
  char ssid[33];
  int32_t rssi;
  bool encrypted;
};

APInfo apList[MAX_APS];
int apCount = 0;

// Pixel buffer for single pixel drawing
uint16_t pixelBuffer[1];

// Function prototypes
void drawPixel(uint16_t x, uint16_t y, uint16_t color);
void drawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg);
void drawString(uint16_t x, uint16_t y, const char* str, uint16_t color, uint16_t bg);
void drawString(uint16_t x, uint16_t y, String str, uint16_t color, uint16_t bg);
void drawSignalBar(uint16_t x, uint16_t y, int rssi, uint16_t width, uint16_t height);
void drawLockIcon(uint16_t x, uint16_t y, uint16_t size, uint16_t color);
void clearScreen(uint16_t color);
void fillRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void sortAPsByRSSI();
int scanWiFi();
void displayWiFiList();

// Draw a single pixel using LCD_addWindow
void drawPixel(uint16_t x, uint16_t y, uint16_t color) {
  if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) return;
  pixelBuffer[0] = color;
  LCD_addWindow(x, y, x, y, pixelBuffer);
}

// Fill rectangle
void fillRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
  if (x1 >= DISPLAY_WIDTH || y1 >= DISPLAY_HEIGHT) return;
  if (x2 >= DISPLAY_WIDTH) x2 = DISPLAY_WIDTH - 1;
  if (y2 >= DISPLAY_HEIGHT) y2 = DISPLAY_HEIGHT - 1;
  
  uint16_t width = x2 - x1 + 1;
  uint16_t height = y2 - y1 + 1;
  uint16_t* buffer = new uint16_t[width];
  
  for (uint16_t i = 0; i < width; i++) {
    buffer[i] = color;
  }
  
  for (uint16_t row = 0; row < height; row++) {
    LCD_addWindow(x1, y1 + row, x2, y1 + row, buffer);
  }
  
  delete[] buffer;
}

// Clear screen
void clearScreen(uint16_t color) {
  fillRect(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, color);
}

// Draw character
void drawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg) {
  if (c < 32 || c > 127) c = 127;
  c -= 32;
  
  for (uint8_t col = 0; col < 5; col++) {
    uint8_t pixel = font5x7[c][col];
    for (uint8_t row = 0; row < 7; row++) {
      if (pixel & (1 << row)) {
        drawPixel(x + col, y + row, color);
      } else if (bg != color) {
        drawPixel(x + col, y + row, bg);
      }
    }
  }
  // Clear space after character
  if (bg != color) {
    for (uint8_t row = 0; row < 7; row++) {
      drawPixel(x + 5, y + row, bg);
    }
  }
}

// Draw string (C-string)
void drawString(uint16_t x, uint16_t y, const char* str, uint16_t color, uint16_t bg) {
  uint16_t cursorX = x;
  while (*str) {
    drawChar(cursorX, y, *str, color, bg);
    cursorX += FONT_WIDTH;
    str++;
  }
}

// Draw string (Arduino String)
void drawString(uint16_t x, uint16_t y, String str, uint16_t color, uint16_t bg) {
  drawString(x, y, str.c_str(), color, bg);
}

// Draw signal strength bar
void drawSignalBar(uint16_t x, uint16_t y, int rssi, uint16_t width, uint16_t height) {
  // RSSI typically ranges from -100 (weak) to -30 (strong)
  int signalPercent = map(rssi, -100, -30, 0, 100);
  signalPercent = constrain(signalPercent, 0, 100);
  
  uint16_t barWidth = (width * signalPercent) / 100;
  
  // Draw background (empty bar)
  fillRect(x, y, x + width - 1, y + height - 1, GRAY);
  
  // Draw filled portion
  if (barWidth > 0) {
    uint16_t barColor;
    if (signalPercent > 70) {
      barColor = GREEN;
    } else if (signalPercent > 40) {
      barColor = YELLOW;
    } else {
      barColor = RED;
    }
    fillRect(x, y, x + barWidth - 1, y + height - 1, barColor);
  }
}

// Draw lock icon (simple 8x8 bitmap)
void drawLockIcon(uint16_t x, uint16_t y, uint16_t size, uint16_t color) {
  const uint8_t lockIcon[8] = {
    0x3C, 0x7E, 0x41, 0x41, 0x7F, 0x7F, 0x7F, 0x7F
  };
  
  for (uint8_t row = 0; row < 8; row++) {
    for (uint8_t col = 0; col < 8; col++) {
      if (lockIcon[row] & (1 << (7 - col))) {
        drawPixel(x + col, y + row, color);
      }
    }
  }
}

// Sort APs by RSSI (strongest first)
void sortAPsByRSSI() {
  for (int i = 0; i < apCount - 1; i++) {
    for (int j = 0; j < apCount - i - 1; j++) {
      if (apList[j].rssi < apList[j + 1].rssi) {
        APInfo temp = apList[j];
        apList[j] = apList[j + 1];
        apList[j + 1] = temp;
      }
    }
  }
}

// Scan WiFi networks
int scanWiFi() {
  apCount = 0;
  int n = WiFi.scanNetworks();
  
  if (n == 0) {
    return 0;
  }
  
  apCount = min(n, MAX_APS);
  
  for (int i = 0; i < apCount; i++) {
    String ssid = WiFi.SSID(i);
    ssid.toCharArray(apList[i].ssid, 33);
    apList[i].rssi = WiFi.RSSI(i);
    apList[i].encrypted = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
  }
  
  WiFi.scanDelete();
  sortAPsByRSSI();
  
  return apCount;
}

// Display WiFi list on screen
void displayWiFiList() {
  clearScreen(BLACK);
  
  // Header
  drawString(5, 5, "WiFi Scanner", WHITE, BLACK);
  drawString(5, 17, "--------------", GRAY, BLACK);
  
  int startY = 32;
  int lineHeight = LINE_HEIGHT;
  int maxLines = (DISPLAY_HEIGHT - startY - 20) / lineHeight;
  
  int displayCount = min(apCount, maxLines);
  
  for (int i = 0; i < displayCount; i++) {
    int yPos = startY + (i * lineHeight);
    
    // Draw SSID (truncate if too long)
    char ssidDisplay[18];
    strncpy(ssidDisplay, apList[i].ssid, 17);
    ssidDisplay[17] = '\0';
    drawString(5, yPos, ssidDisplay, WHITE, BLACK);
    
    // Draw lock icon if encrypted
    if (apList[i].encrypted) {
      drawLockIcon(DISPLAY_WIDTH - 50, yPos + 2, 8, YELLOW);
    }
    
    // Draw signal bar
    drawSignalBar(DISPLAY_WIDTH - 40, yPos + 3, apList[i].rssi, 35, 6);
  }
  
  // Show scan info at bottom
  char infoStr[30];
  sprintf(infoStr, "APs: %d", apCount);
  drawString(5, DISPLAY_HEIGHT - 15, infoStr, GRAY, BLACK);
}

void setup() {
  Serial.begin(115200);
  
  // Initialize display
  LCD_Init();
  clearScreen(BLACK);
  
  // Show startup message
  drawString(10, 100, "WiFi Scanner", WHITE, BLACK);
  drawString(10, 120, "Starting...", GRAY, BLACK);
  
  // Initialize WiFi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  Serial.println("Setup complete");
}

void loop() {
  // Scan for networks
  Serial.println("Scanning...");
  int found = scanWiFi();
  
  if (found > 0) {
    Serial.printf("Found %d networks\n", found);
    displayWiFiList();
  } else {
    clearScreen(BLACK);
    drawString(10, 100, "No networks", WHITE, BLACK);
    drawString(10, 120, "found...", GRAY, BLACK);
  }
  
  // Wait before next scan
  delay(5000);
}