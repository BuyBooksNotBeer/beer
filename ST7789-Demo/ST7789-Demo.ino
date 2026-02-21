#include <WiFi.h>
#include <WebServer.h>
#include "Display_ST7789.h"
#include <math.h>
#include "esp_wifi.h"
#include "driver/temperature_sensor.h"

// --- Simulation Constants ---
#define BALL_RADIUS 5
#define BAR_W 40
#define BAR_H 8
#define BALL_MASS 1.0f
#define BAR_MASS 2.0f

// --- Global State ---
uint16_t* frameBuffer;
WebServer server(80);

temperature_sensor_handle_t temp_sensor = NULL;

struct Object {
  float x, y, vx, vy, angle, av; // x, y, velocity, angle, angular velocity
  uint16_t color;
};

Object ball = {80, 160, 2.5, 3.1, 0, 0, 0xF800}; // Red ball
Object bar = {50, 50, 1.5, 1.2, 0.5, 0.05, 0x07E0}; // Green bar
int contactCount = 0;
bool barPaused = false;
float speedMultiplier = 1.0f;

// --- Helper: Random Color ---
uint16_t randomColor() {
  return ((random(0, 31) << 11) | (random(0, 63) << 5) | random(0, 31));
}

// --- Web Server Handlers ---
void handleRoot() {
  String html = "<h1>Physics Control</h1>";
  html += "<p><a href='/speedUp'><button>Ball Speed +</button></a> ";
  html += "<a href='/speedDown'><button>Ball Speed -</button></a></p>";
  html += "<p><a href='/pause'><button>Toggle Bar Pause</button></a></p>";
  html += "<p><a href='/reset'><button>Reset Sim</button></a></p>";
  server.send(200, "text/html", html);
}

// --- Physics Engine ---
void updatePhysics() {
  // Update Ball
  ball.x += ball.vx * speedMultiplier;
  ball.y += ball.vy * speedMultiplier;

  // Update Bar
  if (!barPaused) {
    bar.x += bar.vx;
    bar.y += bar.vy;
    bar.angle += bar.av;
  }

  // Wall Bounces (Ball)
  if (ball.x <= BALL_RADIUS || ball.x >= LCD_WIDTH - BALL_RADIUS) ball.vx *= -1;
  if (ball.y <= BALL_RADIUS || ball.y >= LCD_HEIGHT - BALL_RADIUS) ball.vy *= -1;

  // Wall Bounces (Bar Corners)
  // Simplified: check bar center + rough radius for bounce
  if (bar.x <= 20 || bar.x >= LCD_WIDTH - 20) bar.vx *= -1;
  if (bar.y <= 20 || bar.y >= LCD_HEIGHT - 20) bar.vy *= -1;

  // --- Ball vs Rotating Bar Collision ---
  // 1. Transform ball to bar's local coordinate system
  float dx = ball.x - bar.x;
  float dy = ball.y - bar.y;
  float localX = dx * cos(-bar.angle) - dy * sin(-bar.angle);
  float localY = dx * sin(-bar.angle) + dy * cos(-bar.angle);

  // 2. Check collision with axis-aligned box
  if (abs(localX) < (BAR_W / 2 + BALL_RADIUS) && abs(localY) < (BAR_H / 2 + BALL_RADIUS)) {
    // Collision detected!
    contactCount++;
    ball.color = randomColor();
    bar.color = randomColor();

    // Simple elastic response: swap some momentum
    float tempVx = ball.vx;
    float tempVy = ball.vy;
    ball.vx = (ball.vx * (BALL_MASS - BAR_MASS) + (2 * BAR_MASS * bar.vx)) / (BALL_MASS + BAR_MASS);
    bar.vx = (bar.vx * (BAR_MASS - BALL_MASS) + (2 * BALL_MASS * tempVx)) / (BALL_MASS + BAR_MASS);
    
    ball.vy *= -1; // Reflect
    bar.av += 0.02; // Add some spin on impact
  }
}

// --- Drawing Functions ---
void clearBuffer() { memset(frameBuffer, 0, LCD_WIDTH * LCD_HEIGHT * 2); }

void drawPixel(int x, int y, uint16_t color) {
  if (x >= 0 && x < LCD_WIDTH && y >= 0 && y < LCD_HEIGHT)
    frameBuffer[y * LCD_WIDTH + x] = color;
}

void drawBall() {
  for (int i = -BALL_RADIUS; i < BALL_RADIUS; i++) {
    for (int j = -BALL_RADIUS; j < BALL_RADIUS; j++) {
      if (i * i + j * j <= BALL_RADIUS * BALL_RADIUS)
        drawPixel(ball.x + i, ball.y + j, ball.color);
    }
  }
}

void drawBar() {
  float c = cos(bar.angle);
  float s = sin(bar.angle);
  for (int i = -BAR_W / 2; i < BAR_W / 2; i++) {
    for (int j = -BAR_H / 2; j < BAR_H / 2; j++) {
      int rx = bar.x + (i * c - j * s);
      int ry = bar.y + (i * s + j * c);
      drawPixel(rx, ry, bar.color);
    }
  }
}

// Simple 5x7 Font Renderer (Scaled to ~15px)
void drawText(int x, int y, String txt) {
  // Simplified: Draws blocky numbers for the counter
  for (int i = 0; i < txt.length(); i++) {
    for (int px = 0; px < 10; px++) {
      for (int py = 0; py < 15; py++) {
        drawPixel(x + (i * 12) + px, y + py, 0xFFFF);
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Allocate Framebuffer
  frameBuffer = (uint16_t*)psramFound() ? (uint16_t*)ps_malloc(LCD_WIDTH * LCD_HEIGHT * 2) : (uint16_t*)malloc(LCD_WIDTH * LCD_HEIGHT * 2);

  Serial.println("ESP32-C6 Temperature Sensor Example");
    
  LCD_Init();
  Set_Backlight(100);

     // Configure temperature sensor
    temperature_sensor_config_t temp_sensor_config = {
        .range_min = 10,
        .range_max = 50,
        .clk_src = TEMPERATURE_SENSOR_CLK_SRC_DEFAULT
    };
    
    // Install and enable
    ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor_config, &temp_sensor));
    ESP_ERROR_CHECK(temperature_sensor_enable(temp_sensor));

  // WiFi AP Setup
  WiFi.softAP("ESP32-Physics-Sim", ""); 
  
  server.on("/", handleRoot);
  server.on("/speedUp", []() { speedMultiplier += 0.2; handleRoot(); });
  server.on("/speedDown", []() { speedMultiplier -= 0.2; handleRoot(); });
  server.on("/pause", []() { barPaused = !barPaused; handleRoot(); });
  server.on("/reset", []() { ball.x = 80; ball.y = 160; contactCount = 0; handleRoot(); });
  server.begin();
}

void loop() {

  float temp;
    esp_err_t result = temperature_sensor_get_celsius(temp_sensor, &temp);
    
    if (result == ESP_OK) {
        Serial.printf("Temperature: %.2f °C / %.2f °F\n", 
                      temp, temp * 1.8 + 32);
    } else {
        Serial.println("Error reading temperature");
    }
    
  server.handleClient();
  
  updatePhysics();
  
  clearBuffer();
  drawBall();
  drawBar();
  
  // Counter & WiFi Signal
  drawText(10, 10, String(contactCount));
  
  // Get RSSI of first station (if any)
  wifi_sta_list_t stationList;
  esp_wifi_ap_get_sta_list(&stationList);
  if (stationList.num > 0) {
     drawText(120, 10, "W"); // Indicate WiFi active
  }

  // Push buffer to LCD
  LCD_addWindow(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, frameBuffer);
  
  delay(1000); // Target ~60fps
}