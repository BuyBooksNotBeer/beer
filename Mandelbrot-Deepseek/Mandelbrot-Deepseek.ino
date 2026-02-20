#include "Display_ST7789.h"

// Mandelbrot settings
#define MAX_ITERATIONS 100
#define ZOOM_FACTOR 1.5
#define ZOOM_STAGES 5

// Fractal viewport parameters
typedef struct {
  double minX;
  double maxX;
  double minY;
  double maxY;
  double centerX;
  double centerY;
  double zoom;
} Viewport;

Viewport viewport;
uint16_t* framebuffer = nullptr;
volatile bool renderComplete = false;
volatile bool newRenderRequest = false;
volatile int currentStage = 0;

// For ESP32-C6 we'll use a single core with cooperative multitasking
unsigned long lastRenderTime = 0;
unsigned long lastDisplayTime = 0;
bool renderingInProgress = false;
int renderY = 0;

// Color mapping function - blue violet theme
uint16_t getColor(int iterations) {
  if (iterations == MAX_ITERATIONS) {
    // Inside Mandelbrot set - black
    return 0x0000;
  }
  
  // Blue violet gradient
  uint8_t r, g, b;
  
  if (iterations < MAX_ITERATIONS / 3) {
    // Dark blue to violet transition
    float t = (float)iterations / (MAX_ITERATIONS / 3);
    r = (uint8_t)(75 * t);      // Increasing red
    g = (uint8_t)(0);           // No green
    b = (uint8_t)(130 + 125 * t); // Blue to violet
  } 
  else if (iterations < 2 * MAX_ITERATIONS / 3) {
    // Violet to light violet
    float t = (float)(iterations - MAX_ITERATIONS / 3) / (MAX_ITERATIONS / 3);
    r = (uint8_t)(75 + 105 * t);    // 75 to 180
    g = (uint8_t)(0 + 50 * t);      // 0 to 50
    b = (uint8_t)(255);              // Full blue
  } 
  else {
    // Light violet to white
    float t = (float)(iterations - 2 * MAX_ITERATIONS / 3) / (MAX_ITERATIONS / 3);
    r = (uint8_t)(180 + 75 * t);    // 180 to 255
    g = (uint8_t)(50 + 205 * t);     // 50 to 255
    b = (uint8_t)(255);              // Full blue
  }
  
  // Convert RGB 8-8-8 to RGB 5-6-5
  return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

// Calculate Mandelbrot for a given point
int calculateMandelbrot(double x0, double y0) {
  double x = 0.0;
  double y = 0.0;
  int iteration = 0;
  
  while (x*x + y*y <= 4.0 && iteration < MAX_ITERATIONS) {
    double xTemp = x*x - y*y + x0;
    y = 2.0*x*y + y0;
    x = xTemp;
    iteration++;
  }
  
  return iteration;
}

// Render a single line of the Mandelbrot set
bool renderNextLine() {
  if (renderY >= LCD_HEIGHT) {
    renderY = 0;
    renderingInProgress = false;
    renderComplete = true;
    return true; // Complete
  }
  
  int y = renderY;
  for (int x = 0; x < LCD_WIDTH; x++) {
    // Map pixel coordinates to complex plane
    double real = viewport.minX + (x * (viewport.maxX - viewport.minX) / LCD_WIDTH);
    double imag = viewport.minY + (y * (viewport.maxY - viewport.minY) / LCD_HEIGHT);
    
    int iterations = calculateMandelbrot(real, imag);
    framebuffer[y * LCD_WIDTH + x] = getColor(iterations);
  }
  
  renderY++;
  
  // Display progress every few lines
  if (renderY % 10 == 0) {
    // Update display with partial render
    LCD_addWindow(0, 0, LCD_WIDTH - 1, renderY - 1, framebuffer);
  }
  
  return false; // Not complete yet
}

// Update viewport for next zoom stage
void updateViewportForZoom() {
  // Calculate new zoomed range (zoom in towards center)
  double rangeX = viewport.maxX - viewport.minX;
  double rangeY = viewport.maxY - viewport.minY;
  
  double newRangeX = rangeX / ZOOM_FACTOR;
  double newRangeY = rangeY / ZOOM_FACTOR;
  
  viewport.minX = viewport.centerX - newRangeX / 2;
  viewport.maxX = viewport.centerX + newRangeX / 2;
  viewport.minY = viewport.centerY - newRangeY / 2;
  viewport.maxY = viewport.centerY + newRangeY / 2;
  
  viewport.zoom *= ZOOM_FACTOR;
  
  // Ensure viewport stays within bounds (prevent going off-screen)
  if (viewport.minX < -2.5) {
    double shift = -2.5 - viewport.minX;
    viewport.minX += shift;
    viewport.maxX += shift;
    viewport.centerX += shift / 2;
  }
  if (viewport.maxX > 1.5) {
    double shift = viewport.maxX - 1.5;
    viewport.minX -= shift;
    viewport.maxX -= shift;
    viewport.centerX -= shift / 2;
  }
  if (viewport.minY < -1.5) {
    double shift = -1.5 - viewport.minY;
    viewport.minY += shift;
    viewport.maxY += shift;
    viewport.centerY += shift / 2;
  }
  if (viewport.maxY > 1.5) {
    double shift = viewport.maxY - 1.5;
    viewport.minY -= shift;
    viewport.maxY -= shift;
    viewport.centerY -= shift / 2;
  }
}

// Initialize viewport for full Mandelbrot set
void initViewport() {
  viewport.minX = -2.5;
  viewport.maxX = 1.5;
  viewport.minY = -1.5;
  viewport.maxY = 1.5;
  viewport.centerX = (viewport.minX + viewport.maxX) / 2;
  viewport.centerY = (viewport.minY + viewport.maxY) / 2;
  viewport.zoom = 1.0;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Initializing Display...");
  LCD_Init();
  
  // Allocate framebuffer in PSRAM if available, otherwise in normal RAM
  if (psramFound()) {
    framebuffer = (uint16_t*)ps_malloc(LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t));
    Serial.println("Using PSRAM for framebuffer");
  } else {
    framebuffer = (uint16_t*)malloc(LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t));
    Serial.println("Using internal RAM for framebuffer");
  }
  
  if (!framebuffer) {
    Serial.println("FATAL: Failed to allocate framebuffer!");
    while (1) { delay(1000); }
  }
  
  // Initialize viewport
  initViewport();
  
  // Set backlight to full brightness
  Set_Backlight(100);
  
  // Clear framebuffer
  memset(framebuffer, 0, LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t));
  
  // Start first render
  renderingInProgress = true;
  renderY = 0;
  renderComplete = false;
  
  Serial.println("Setup complete - Rendering Mandelbrot...");
}

// State machine for zoom sequence
enum DisplayState {
  STATE_RENDERING,
  STATE_WAITING,
  STATE_ZOOMING,
  STATE_RESETTING
};

DisplayState currentState = STATE_RENDERING;
unsigned long stateStartTime = 0;

void loop() {
  unsigned long currentTime = millis();
  
  switch (currentState) {
    case STATE_RENDERING:
      if (renderingInProgress) {
        // Continue rendering line by line
        bool complete = renderNextLine();
        if (complete) {
          // Full render complete
          LCD_addWindow(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, framebuffer);
          currentState = STATE_WAITING;
          stateStartTime = currentTime;
          Serial.printf("Render complete. Stage %d of %d\n", currentStage + 1, ZOOM_STAGES);
        }
      }
      break;
      
    case STATE_WAITING:
      // Wait 1 second before next action
      if (currentTime - stateStartTime >= 1000) {
        if (currentStage < ZOOM_STAGES - 1) {
          // Move to next zoom stage
          currentState = STATE_ZOOMING;
        } else {
          // All stages complete, reset
          currentState = STATE_RESETTING;
        }
      }
      break;
      
    case STATE_ZOOMING:
      // Update viewport for next zoom stage
      updateViewportForZoom();
      currentStage++;
      
      // Start new render
      renderingInProgress = true;
      renderY = 0;
      renderComplete = false;
      currentState = STATE_RENDERING;
      Serial.printf("Zooming to stage %d\n", currentStage + 1);
      break;
      
    case STATE_RESETTING:
      // Reset to initial viewport
      initViewport();
      currentStage = 0;
      
      // Start new render
      renderingInProgress = true;
      renderY = 0;
      renderComplete = false;
      currentState = STATE_RENDERING;
      Serial.println("Resetting to full view");
      break;
  }
  
  // Small delay to prevent watchdog issues
  delay(1);
}