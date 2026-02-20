#include "Display_ST7789.h"

#define SPI_WRITE(_dat)         SPI.transfer(_dat)
#define SPI_WRITE_Word(_dat)    SPI.transfer16(_dat)

void SPI_Init()
{
  // Initialize SPI with hardware CS pin management
  SPI.begin(EXAMPLE_PIN_NUM_SCLK, EXAMPLE_PIN_NUM_MISO, EXAMPLE_PIN_NUM_MOSI, EXAMPLE_PIN_NUM_LCD_CS);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, HIGH);  // Deselect display initially
}

void LCD_WriteCommand(uint8_t Cmd)
{
  SPI.beginTransaction(SPISettings(SPIFreq, MSBFIRST, SPI_MODE0));
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, LOW);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_DC, LOW);
  SPI_WRITE(Cmd);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, HIGH);
  SPI.endTransaction();
}

void LCD_WriteData(uint8_t Data)
{
  SPI.beginTransaction(SPISettings(SPIFreq, MSBFIRST, SPI_MODE0));
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, LOW);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_DC, HIGH);
  SPI_WRITE(Data);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, HIGH);
  SPI.endTransaction();
}

void LCD_WriteData_Word(uint16_t Data)
{
  SPI.beginTransaction(SPISettings(SPIFreq, MSBFIRST, SPI_MODE0));
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, LOW);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_DC, HIGH);
  SPI_WRITE_Word(Data);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, HIGH);
  SPI.endTransaction();
}

void LCD_Reset(void)
{
  digitalWrite(EXAMPLE_PIN_NUM_LCD_RST, LOW);
  delay(10);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_RST, HIGH);
  delay(120);  // Required wake-up delay per ST7789 spec
}

void LCD_Init(void)
{
  pinMode(EXAMPLE_PIN_NUM_LCD_CS, OUTPUT);
  pinMode(EXAMPLE_PIN_NUM_LCD_DC, OUTPUT);
  pinMode(EXAMPLE_PIN_NUM_LCD_RST, OUTPUT);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, HIGH);
  
  Backlight_Init();
  SPI_Init();
  LCD_Reset();
  
  //************* Start Initial Sequence **********//
  LCD_WriteCommand(0x11);  // Sleep out
  delay(120);
  
  LCD_WriteCommand(0x36);  // Memory Data Access Control
  if (HORIZONTAL)
    LCD_WriteData(0x00);
  else
    LCD_WriteData(0x70);   // Vertical mode with RGB order
  
  LCD_WriteCommand(0x3A);  // Interface Pixel Format
  LCD_WriteData(0x05);     // 16-bit color (65K)
  
  LCD_WriteCommand(0xB0);  // Display Inversion
  LCD_WriteData(0x00);
  LCD_WriteData(0xE8);
  
  LCD_WriteCommand(0xB2);  // Porch Setting
  LCD_WriteData(0x0C);
  LCD_WriteData(0x0C);
  LCD_WriteData(0x00);
  LCD_WriteData(0x33);
  LCD_WriteData(0x33);
  
  LCD_WriteCommand(0xB7);  // Gate Control
  LCD_WriteData(0x35);
  
  LCD_WriteCommand(0xBB);  // VCOM Setting
  LCD_WriteData(0x35);
  
  LCD_WriteCommand(0xC0);  // LCM Control
  LCD_WriteData(0x2C);
  
  LCD_WriteCommand(0xC2);  // VDV and VRH Command Enable
  LCD_WriteData(0x01);
  
  LCD_WriteCommand(0xC3);  // VRH Set
  LCD_WriteData(0x13);
  
  LCD_WriteCommand(0xC4);  // VDV Set
  LCD_WriteData(0x20);
  
  LCD_WriteCommand(0xC6);  // Frame Rate Control
  LCD_WriteData(0x0F);
  
  LCD_WriteCommand(0xD0);  // Power Control 1
  LCD_WriteData(0xA4);
  LCD_WriteData(0xA1);
  
  LCD_WriteCommand(0xD6);  // VAP/VAN Enable
  LCD_WriteData(0xA1);
  
  LCD_WriteCommand(0xE0);  // Positive Voltage Gamma Control
  LCD_WriteData(0xF0);
  LCD_WriteData(0x00);
  LCD_WriteData(0x04);
  LCD_WriteData(0x04);
  LCD_WriteData(0x04);
  LCD_WriteData(0x05);
  LCD_WriteData(0x29);
  LCD_WriteData(0x33);
  LCD_WriteData(0x3E);
  LCD_WriteData(0x38);
  LCD_WriteData(0x12);
  LCD_WriteData(0x12);
  LCD_WriteData(0x28);
  LCD_WriteData(0x30);
  
  LCD_WriteCommand(0xE1);  // Negative Voltage Gamma Control
  LCD_WriteData(0xF0);
  LCD_WriteData(0x07);
  LCD_WriteData(0x0A);
  LCD_WriteData(0x0D);
  LCD_WriteData(0x0B);
  LCD_WriteData(0x07);
  LCD_WriteData(0x28);
  LCD_WriteData(0x33);
  LCD_WriteData(0x3E);
  LCD_WriteData(0x36);
  LCD_WriteData(0x14);
  LCD_WriteData(0x14);
  LCD_WriteData(0x29);
  LCD_WriteData(0x32);
  
  LCD_WriteCommand(0x21);  // Display Inversion On
  LCD_WriteCommand(0x29);  // Display On
}

/******************************************************************************
function: Set the cursor position with proper offset handling
parameter:
Xstart: Start x coordinate
Ystart: Start y coordinate
Xend  : End x coordinate
Yend  : End y coordinate
******************************************************************************/
void LCD_SetCursor(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend)
{
  if (HORIZONTAL) {
    // Horizontal mode: X = columns, Y = rows
    LCD_WriteCommand(0x2A);  // Column address set
    uint16_t x1 = Xstart + Offset_X;
    uint16_t x2 = Xend + Offset_X;
    LCD_WriteData(x1 >> 8);
    LCD_WriteData(x1 & 0xFF);
    LCD_WriteData(x2 >> 8);
    LCD_WriteData(x2 & 0xFF);
    
    LCD_WriteCommand(0x2B);  // Page address set
    uint16_t y1 = Ystart + Offset_Y;
    uint16_t y2 = Yend + Offset_Y;
    LCD_WriteData(y1 >> 8);
    LCD_WriteData(y1 & 0xFF);
    LCD_WriteData(y2 >> 8);
    LCD_WriteData(y2 & 0xFF);
  }
  else {
    // Vertical mode (your display's mode): X/Y swapped in hardware
    LCD_WriteCommand(0x2A);  // Column address set (maps to Y in vertical mode)
    uint16_t x1 = Ystart + Offset_Y;
    uint16_t x2 = Yend + Offset_Y;
    LCD_WriteData(x1 >> 8);
    LCD_WriteData(x1 & 0xFF);
    LCD_WriteData(x2 >> 8);
    LCD_WriteData(x2 & 0xFF);
    
    LCD_WriteCommand(0x2B);  // Page address set (maps to X in vertical mode)
    uint16_t y1 = Xstart + Offset_X;
    uint16_t y2 = Xend + Offset_X;
    LCD_WriteData(y1 >> 8);
    LCD_WriteData(y1 & 0xFF);
    LCD_WriteData(y2 >> 8);
    LCD_WriteData(y2 & 0xFF);
  }
  LCD_WriteCommand(0x2C);  // Memory write command
}

/******************************************************************************
function: Refresh the image in an area (fixed stack overflow + coordinate bugs)
parameter:
Xstart: Start x coordinate
Ystart: Start y coordinate
Xend  : End x coordinate
Yend  : End y coordinate
color : Pointer to RGB565 pixel data
******************************************************************************/
void LCD_addWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint16_t* color)
{          
  uint16_t Show_Width = Xend - Xstart + 1;
  uint16_t Show_Height = Yend - Ystart + 1;
  uint32_t numBytes = Show_Width * Show_Height * sizeof(uint16_t);
  
  LCD_SetCursor(Xstart, Ystart, Xend, Yend);

  // Use the transaction-safe SPI write without allocating a massive stack buffer
  SPI.beginTransaction(SPISettings(SPIFreq, MSBFIRST, SPI_MODE0));
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, LOW);  
  digitalWrite(EXAMPLE_PIN_NUM_LCD_DC, HIGH);  
  
  // We send the data, but pass NULL for the receive buffer so no memory is allocated
  SPI.transferBytes((uint8_t*)color, NULL, numBytes);

  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, HIGH);  
  SPI.endTransaction();
}

// Backlight control using Arduino-compatible analogWrite (works on all ESP32 variants)
void Backlight_Init(void)
{
  pinMode(EXAMPLE_PIN_NUM_BK_LIGHT, OUTPUT);
  analogWrite(EXAMPLE_PIN_NUM_BK_LIGHT, 255);  // Full brightness (8-bit PWM)
}

void Set_Backlight(uint8_t Light)
{
  if (Light > 100) Light = 100;
  
  // Scale 0-100% to 0-255 (8-bit PWM resolution)
  uint8_t pwm_value = (Light * 255) / 100;
  analogWrite(EXAMPLE_PIN_NUM_BK_LIGHT, pwm_value);
}