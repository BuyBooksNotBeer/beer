#include "Display_ST7789.h"
#include <esp_heap_caps.h>

// Display dimensions
#define DISPLAY_WIDTH  172
#define DISPLAY_HEIGHT 320

// Mandelbrot parameters
#define MAX_ITERATIONS 120
#define ZOOM_FACTOR    0.70f
#define MAX_ZOOM_LEVELS 10

// Complex plane boundaries - initial view
float real_min = -2.4f;
float real_max = 1.0f;
float imag_min = -1.7f;
float imag_max = 1.7f;
int current_zoom_level = 0;

// Blue-violet color palette generator (RGB565)
uint16_t getColor(int iterations) {
  if (iterations >= MAX_ITERATIONS) {
    return 0x0000; // Black for points inside the set
  }
  
  float t = (float)iterations / MAX_ITERATIONS;
  
  // Smooth gradient: deep blue -> violet -> magenta-blue
  float rf = 60.0f + 120.0f * t;
  float gf = 20.0f + 80.0f * t * t;
  float bf = 150.0f + 80.0f * (1.0f - t);
  
  if (rf > 255.0f) rf = 255.0f;
  if (gf > 255.0f) gf = 255.0f;
  if (bf > 255.0f) bf = 255.0f;
  
  uint8_t r = (uint8_t)rf;
  uint8_t g = (uint8_t)gf;
  uint8_t b = (uint8_t)bf;
  
  // Convert RGB888 to RGB565
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// Mandelbrot set calculation
int mandelbrot(float real, float imag) {
  float zr = 0.0f, zi = 0.0f;
  int i = 0;
  
  while (i < MAX_ITERATIONS && (zr * zr + zi * zi) <= 4.0f) {
    float temp = zr * zr - zi * zi + real;
    zi = 2.0f * zr * zi + imag;
    zr = temp;
    i++;
  }
  
  return i;
}

// Single-core rendering function
void renderMandelbrot() {
  // Allocate framebuffer in internal RAM (critical for SPI performance)
  uint16_t* framebuffer = (uint16_t*)heap_caps_malloc(
    DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t), 
    MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT
  );
  
  if (!framebuffer) {
    Serial.println("ERROR: Framebuffer allocation failed!");
    return;
  }
  
  float real_scale = (real_max - real_min) / DISPLAY_WIDTH;
  float imag_scale = (imag_max - imag_min) / DISPLAY_HEIGHT;
  
  // Render entire frame sequentially
  for (int y = 0; y < DISPLAY_HEIGHT; y++) {
    float imag = imag_min + y * imag_scale;
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
      float real = real_min + x * real_scale;
      int iterations = mandelbrot(real, imag);
      framebuffer[y * DISPLAY_WIDTH + x] = getColor(iterations);
    }
  }
  
  // Transfer framebuffer to display
  LCD_addWindow(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, framebuffer);
  
  // Clean up
  heap_caps_free(framebuffer);
}

// Smart zoom with dynamic repositioning to keep features visible
void zoomMandelbrot() {
  // Target interesting regions at different zoom levels
  float target_real, target_imag;
  
  if (current_zoom_level < 3) {
    target_real = -0.5f;    // Main cardioid
    target_imag = 0.0f;
  } else if (current_zoom_level < 6) {
    target_real = -0.75f;   // Seahorse valley
    target_imag = 0.1f;
  } else {
    target_real = -0.745f;  // Mini-bulb region
    target_imag = 0.113f;
  }
  
  // Calculate current center
  float current_center_real = (real_min + real_max) * 0.5f;
  float current_center_imag = (imag_min + imag_max) * 0.5f;
  
  // Smooth transition toward target region
  float blend = (current_zoom_level % 3) * 0.35f + 0.2f;
  if (blend > 1.0f) blend = 1.0f;
  
  float new_center_real = current_center_real * (1.0f - blend) + target_real * blend;
  float new_center_imag = current_center_imag * (1.0f - blend) + target_imag * blend;
  
  // Apply zoom while maintaining aspect ratio
  float real_range = (real_max - real_min) * ZOOM_FACTOR;
  float imag_range = (imag_max - imag_min) * ZOOM_FACTOR;
  
  // Reposition to keep features centered
  real_min = new_center_real - real_range * 0.5f;
  real_max = new_center_real + real_range * 0.5f;
  imag_min = new_center_imag - imag_range * 0.5f;
  imag_max = new_center_imag + imag_range * 0.5f;
  
  current_zoom_level++;
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\nESP32-C6 Mandelbrot Fractal Renderer (Single-Core)");
  Serial.println("Using ST7789 display (172x320)");
  
  // Initialize display
  LCD_Init();
  Set_Backlight(100);  // Full brightness
  
  // Clear display to black
  uint16_t* clear_buf = (uint16_t*)heap_caps_malloc(DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t), MALLOC_CAP_INTERNAL);
  if (clear_buf) {
    for (int i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++) {
      clear_buf[i] = 0x0000;
    }
    LCD_addWindow(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, clear_buf);
    heap_caps_free(clear_buf);
  }
  
  Serial.println("Initialization complete. Starting render loop...");
}

void loop() {
  Serial.printf("Zoom level %d: Rendering fractal... ", current_zoom_level);
  unsigned long start = millis();
  
  renderMandelbrot();
  
  Serial.printf("done (%lums)\n", millis() - start);
  
  // Wait 1 second before next zoom
  delay(1000);
  
  // Zoom or reset
  if (current_zoom_level < MAX_ZOOM_LEVELS) {
    zoomMandelbrot();
  } else {
    Serial.println("\n*** Completed all zoom levels - resetting view ***\n");
    real_min = -2.4f;
    real_max = 1.0f;
    imag_min = -1.7f;
    imag_max = 1.7f;
    current_zoom_level = 0;
    delay(2000); // Pause before restart
  }
}