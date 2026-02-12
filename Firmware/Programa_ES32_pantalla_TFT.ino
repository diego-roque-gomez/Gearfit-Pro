#include <TFT_eSPI.h>
#include <SPI.h>
#include <pgmspace.h>

#include "logo_300x120.h"
#include "gears_100x80.h"
#include "bike_140x80.h"

TFT_eSPI tft = TFT_eSPI();

#define RX2_PIN 16
#define TX2_PIN 17
#define SERIAL_BAUD 115200

int cursorY = 60;

bool inResultScreen = false;

void drawImageRemoveBlack(int x, int y, int w, int h, const uint16_t *img) 
{
  for (int j = 0; j < h; j++) 
  {
    for (int i = 0; i < w; i++) 
    {
      uint16_t pix = pgm_read_word(&img[j * w + i]);
      if (pix != 0x0000)
        tft.drawPixel(x + i, y + j, pix);
    }
  }
}

void fadeInImageRemoveBlack(int x, int y, int w, int h, const uint16_t *img, int totalMs) 
{
  const int steps = 8;
  int stepMs = totalMs / steps;

  uint16_t *buf = (uint16_t*)malloc(w * h * 2);
  if (!buf) return;

  for (uint32_t i = 0; i < (uint32_t)(w*h); i++)
    buf[i] = pgm_read_word(img + i);

  for (int s = 1; s <= steps; s++) 
  {
    float f = (float)s / steps;
    for (uint32_t i = 0; i < (uint32_t)(w*h); i++) 
    {
      uint16_t pix = buf[i];
      if (pix == 0x0000) continue;

      uint8_t r = ((pix >> 11) & 0x1F) * f;
      uint8_t g = ((pix >> 5)  & 0x3F) * f;
      uint8_t b = (pix & 0x1F)         * f;

      uint16_t newpix = (r << 11) | (g << 5) | b;

      int px = i % w;
      int py = i / w;
      tft.drawPixel(x + px, y + py, newpix);
    }
    delay(stepMs);
  }

  free(buf);
}

void fadeInImageCenteredRemoveBlack(const uint16_t *img, int w, int h, int totalMs, int offsetY) 
{
  int x = (480 - w) / 2;
  fadeInImageRemoveBlack(x, offsetY, w, h, img, totalMs);
}

void drawPortada() 
{
  tft.fillScreen(TFT_BLACK);

  fadeInImageRemoveBlack(10, 10, gears_width, gears_height, gears_bitmap, 2000);

  fadeInImageRemoveBlack(320, 40, bike_width, bike_height, bike_bitmap, 2000);

  fadeInImageCenteredRemoveBlack(logo_bitmap, logo_width, logo_height, 2000, 105);

  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(60, 220);
  tft.print("GearCentral - Medidas de casco");

  delay(4000);
}

void initResultScreen() 
{
  inResultScreen = true;

  tft.fillScreen(TFT_BLACK);

  // logo pequeño arriba izq
  drawImageRemoveBlack(10, -10, logo_width, logo_height, logo_bitmap);

  // engranes arriba derecha
  drawImageRemoveBlack(480 - gears_width - 10, 10, gears_width, gears_height, gears_bitmap);

  // título
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.setTextSize(3);
  tft.setCursor(10, logo_height - 20);
  tft.println("Resultados:");

  cursorY = logo_height + 35;
}

void printResultLine(String txt, uint16_t color = TFT_WHITE, uint8_t size = 2) 
{
  if (cursorY > 300) 
  {
    tft.fillRect(0, logo_height + 60, 480, 260, TFT_BLACK);
    cursorY = logo_height + 60;
  }

  tft.setTextColor(color, TFT_BLACK);
  tft.setTextSize(size);
  tft.setCursor(10, cursorY);
  tft.println(txt);

  cursorY += (size == 2 ? 24 : 32);
}

String serialLine = "";
bool haveNewLine = false;

void processLine(String line) 
{
  if (!inResultScreen) return;

  line.trim();
  if (line.length() == 0) return;

  if (line.startsWith("Tipo de cráneo:")) 
  {
    printResultLine(line, TFT_ORANGE, 2);
  }
  else if (line.startsWith("Talla:")) 
  {
    printResultLine(line, TFT_WHITE, 3);
  }
  else 
  {
    printResultLine(line, TFT_WHITE, 2);
  }
}

void setup() 
{
  Serial.begin(115200);
  Serial2.begin(SERIAL_BAUD, SERIAL_8N1, RX2_PIN, TX2_PIN);

  tft.init();
  tft.setRotation(1);

  drawPortada();
  initResultScreen();
}

void loop() 
{
  while (Serial2.available()) 
  {
    char c = Serial2.read();

    if (c == '\n') 
    {
      haveNewLine = true;
      break;
    }
    else if (c != '\r') 
    {
      serialLine += c;
    }
  }

  if (haveNewLine) 
  {
    processLine(serialLine);
    serialLine = "";
    haveNewLine = false;
  }
}
