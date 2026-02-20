#include <Arduino.h>
#include "Display_ST7789.h"

// Define our base colors (RGB565 format)
// Blue Violet is roughly RGB(138, 43, 226). 
// Converted to RGB565: (138>>3)<<11 | (43>>2)<<5 | (226>>3)
#define COLOR_BLUE_VIOLET 0x895C
#define COLOR_BLACK       0x0000

// Zoom and coordinate tracking
double zoom = 1.0;
double currentX = -0.5; // Starting center point (X)
double currentY = 0.0;  // Starting center point (Y)

// We will pan towards this detailed "edge" area so the fractal never leaves the screen
const double targetX = -0.743643887037158; 
const double targetY = 0.131825904205311;

int max_iterations = 40;

void setup() {
  Serial.begin(115200);
  
  // Initialize the LCD using your provided library function
  LCD_Init(); 
}

void loop() {
  // We allocate memory for a single row to prevent stack overflow in LCD_addWindow
  uint16_t lineBuffer[LCD_WIDTH]; 

  // Dynamically pan the camera towards the target edge as we loop
  currentX += (targetX - currentX) * 0.15;
  currentY += (targetY - currentY) * 0.15;

  // Calculate the complex plane dimensions based on the current zoom
  double w = 3.0 / zoom;
  double h = (3.0 * LCD_HEIGHT / LCD_WIDTH) / zoom;
  
  double xMin = currentX - w / 2.0;
  double yMin = currentY - h / 2.0;
  
  // Render the fractal
  for (int py = 0; py < LCD_HEIGHT; py++) {
    double cy = yMin + py * (h / LCD_HEIGHT);
    
    for (int px = 0; px < LCD_WIDTH; px++) {
      double cx = xMin + px * (w / LCD_WIDTH);
      
      double zx = 0;
      double zy = 0;
      int iter = 0;
      
      // Escape time algorithm
      while (zx * zx + zy * zy <= 4.0 && iter < max_iterations) {
        double tmp = zx * zx - zy * zy + cx;
        zy = 2.0 * zx * zy + cy;
        zx = tmp;
        iter++;
      }
      
      // Color mapping
      if (iter == max_iterations) {
        // Inside the Mandelbrot set
        lineBuffer[px] = COLOR_BLACK; 
      } else {
        // Outside the set (background): Scale the Blue-Violet color by iterations for depth
        uint16_t r = 138 * iter / max_iterations;
        uint16_t g = 43  * iter / max_iterations;
        uint16_t b = 226 * iter / max_iterations;
        lineBuffer[px] = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
      }
    }
    
    // Push the current row to the display using your library's window function
    LCD_addWindow(0, py, LCD_WIDTH - 1, py, lineBuffer); 
  }
  
  // Fulfill the 1-second delay requirement before zooming
  delay(1000); 
  
  // Increase zoom and add iterations for more detail as we get closer
  zoom *= 1.4; 
  max_iterations += 4; 
  
  // Reset sequence if we zoom too far (prevents floating-point precision breakdown)
  if (zoom > 50000.0) {
    zoom = 1.0;
    currentX = -0.5;
    currentY = 0.0;
    max_iterations = 40;
  }
}