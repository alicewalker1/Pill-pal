// Change these defines for where you attach the screen and touchpanel.

// The CLK/MOSI/MISO pins must be the SPI pins, the others can be anything

#define TFT_DC 4
#define TFT_CS 15
#define TFT_CLK 14
#define TFT_MOSI 13
#define TFT_MISO 12
#define TOUCH_CS 5
#define TOUCH_IRQ 18  

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <XPT2046_Touchscreen.h>

XPT2046_Touchscreen ts(TOUCH_CS);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

typedef struct BUTTON_S {
  char used = 0; 
  String text; //string to match
 // byte status;
  int x;
  int y;
  int w;
  int h;
  byte updated = 1;
  byte on;
  void (*action)(int, String);
} BUTTON;

//BUTTON*** buttons;

BUTTON buttons[BUTTONCOUNT];


void setup() {

  Serial.begin(115200);
  Serial.println("TFT/Touch demo");
  ts_init();
  init_screen();

  int const numX = 5;
  int const numY = 2;
  tileButtons(numX,numY,0,120,320,240-120);

  buttons[0].text = "1";
  buttons[1].text = "2";
  buttons[2].text = "3";
  buttons[3].text = "4";
  buttons[4].text = "5";
  buttons[5].text = "6";
  buttons[6].text = "7";
  buttons[7].text = "8";

  buttons[0].action = press;
  buttons[1].action = press;
  buttons[2].action = press;
  buttons[3].action = press;
  buttons[4].action = press;
  buttons[5].action = press;
  buttons[6].action = press;
  buttons[7].action = press;
  
 
  SPI.setClockDivider(SPI_CLOCK_DIV16); // 16/4 MHz
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.println("Hello World");
  drawButtons();
}

int cursorx = 0, cursory = 0;
unsigned long color = 0xFFFF;
byte orientation;
byte size;

void init_screen() {
  tft.begin();

  #ifdef TFT_BACKLIGHT
  pinMode(TFT_BACKLIGHT,OUTPUT);
  digitalWrite(TFT_BACKLIGHT,HIGH);
  #endif

  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE); tft.setTextSize(3);
  tft.setRotation(3);
  tft.println("BOOTING ...");
}


void scrnDraw() {
  //  scrnBlank(NULL,""); //yes no?
  //  or are we going to use local clears

  tft.setTextSize(1); //10px

  int tftofset = 0; //used when no sensor found
  int i = 0;
  for ( i = 0; i < 8; i++) {
    //  if (relayPower[i]) { //only plot channels that have a sensor
    float busvoltage = ina226[i]->readBusVoltage();
    float current_A = ina226[i]->readShuntCurrent()+ 0.001;
    //P1A colour
    if (relayActive[i]) {
      tft.setTextColor(ILI9341_GREEN);
    } else {
      tft.setTextColor(ILI9341_RED);
    }

    tft.fillRect((tftofset) * 31 + 5, 10 + powerOfset, 40, 10, ILI9341_BLACK);
    tft.setCursor((tftofset) * 31 + 5, 10 + powerOfset);
    tft.print("P");
    tft.print(i + 1);
    tft.print(relayActive[i] ? 'A' : 'N');

    if ( busvoltage > 20) {
      tft.setTextColor(ILI9341_GREEN);
    } else {
      tft.setTextColor(ILI9341_RED);
    }


    tft.fillRect((tftofset) * 31 + 5, 22 + powerOfset, 40, 10, ILI9341_BLACK);
    tft.setCursor((tftofset) * 31 + 5, 22 + powerOfset);
    tft.print(busvoltage, 1);

    if (current_A * busvoltage < 30) { //watts
      tft.setTextColor(ILI9341_GREEN);
    } else {
      tft.setTextColor(ILI9341_RED);
      if (current_A * busvoltage > 40) {
        relay(i,"n");
      }
    }
    tft.fillRect((tftofset) * 31 + 5, 34 + powerOfset, 40, 10, ILI9341_BLACK);
    tft.setCursor((tftofset) * 31 + 5, 34 + powerOfset);
    tft.print(abs(current_A), 2);
    tftofset++;
    // }
  }
  tft.setTextColor(ILI9341_GREEN);

  tft.fillRect((tftofset) * 31 + 5, 22 + powerOfset, 40, 10, ILI9341_BLACK);
  tft.setCursor((tftofset) * 31 + 5, 22 + powerOfset);
  tft.print("V");


  tft.fillRect((tftofset) * 31 + 5, 34 + powerOfset, 40, 10, ILI9341_BLACK);
  tft.setCursor((tftofset) * 31 + 5, 34 + powerOfset);
  tft.print("Amps");

  for (tftofset++; tftofset < 8; tftofset++) {
    tft.fillRect((tftofset) * 31 + 5, 22 + powerOfset, 40, 10, ILI9341_BLACK);
    tft.fillRect((tftofset) * 31 + 5, 34 + powerOfset, 40, 10, ILI9341_BLACK);
  }

  tft.setTextSize(size);
}

#define TS_MINX 140
#define TS_MAXX 900
#define TS_MINY 120
#define TS_MAXY 940

void ts_init() {
  ts.begin();
  ts.setRotation(1);

  addCommand(command_count++, "BC", &createButton, -1, NULL);//create button "bc x y w h 'text'"  bc 10 100 100 40 'text'
  addCommand(command_count++, "BS", &setButton, -1, NULL);//create button "bc x y ?"  bc 10 100 1
  //will return BH
  addCommand(command_count++, "BR", &removeButton, -1, NULL);//remove button at x y "BR x y       br 10 100
}

void setButton(int z, String command) {
  int x = command.toInt();
  int index = command.indexOf(" ") + 1;
  int y = command.substring(index).toInt();
  index = command.indexOf(" ", index) + 1;

  for (int i = 0; i < BUTTONCOUNT ; i++) {
    if (buttons[i].x == x && buttons[i].y == y) {
      buttons[i].updated = 1;
      buttons[i].on = command.substring(index).toInt();
    }
  }
}


void createButton(int index, String command)
{
  for (int i = 0; i < BUTTONCOUNT ; i++) {
    if (buttons[i].used == false) {

      int x = command.toInt();
      int index = command.indexOf(" ") + 1;
      int y = command.substring(index).toInt();

      index = command.indexOf(" ", index) + 1;
      int w = command.substring(index).toInt();

      index = command.indexOf(" ", index) + 1;
      int h = command.substring(index).toInt();

      index = command.indexOf(" ", index) + 1;
      command.replace('/','\n');    
      setupButton(&buttons[i], x, y, w, h, command.substring(index));
      return;
    }
  }
  tft.setCursor(cursorx, cursory);
}

int touchHeld(int duration) {
  unsigned long start = millis();
  while (millis() - start < duration) {
    TS_Point p = ts.getPoint();
    p.x = convertX(p.x);
    p.y = convertY(p.y);

    if (p.x < 0 || p.y < 0) {
      return false;
    }
    if (!ts.touched()) {
      return false;
    }
  }
  return true;
}


//Creates a new button
void setupButton(BUTTON* btn, int x, int y, int w, int h, String text) {
  btn->used = true;
  btn->on = false;
  btn->x = x;
  btn->y = y;
  btn->w = w;
  btn->h = h;
  btn->text = text; //might fail?
  btn->updated = 1; // make sure we draw it
  //  btn->action = NULL;
}

//creates an array of buttons form x,y to x+w, y+h where each button is a % of that.
  void tileButtons(int numX, int numY, int posX, int posY, int w, int h) {
  int btnWidth = w / numX;
  int btnHeight = h / numY;
  
    for (int x = 0; x < numX; x++) {
      for (int y = 0; y < numY; y++) {
          //Serial.print("btn x: ");Serial.print(x);Serial.print("y: ");Serial.print(y); Serial.println();
        setupButton(&buttons[x+y*numX], btnWidth * x + posX, btnHeight * y + posY, btnWidth, btnHeight,"");
      }
    }
  }

/*
void allOff(int index , String command) {
  Serial.print("allOff "); Serial.println();
  for (int i = 0; i < BUTTONCOUNT; i++) {
    buttons[i].on = 0;
    buttons[i].updated = 1;
    if (buttons[i].action != NULL) {
         buttons[i].action(i, buttons[i].on ? "A" : "N");
    }
  }
}
*/



/*
void drawCross(int x, int y) {
  tft.drawLine(x - 5, y, x + 5, y, ILI9341_GREEN);
  tft.drawLine(x, y - 5, x, y + 5, ILI9341_GREEN);
}
*/
int convertX(int xPos) {
  return map(xPos, 481, 3798, 10, 310); //calibration points
}

int convertY(int yPos) {
  return map(yPos, 422, 3543, 10, 230); //calibration points
}

//
//void calibrate_screen() {
//  tft.fillScreen(ILI9341_BLACK);
//
//  drawCross(10, 10);
//  while (touchHeld(500));
//  while (!touchHeld(500));
////  eepromData.touchMIN = ts.getPoint();
//  tft.fillScreen(ILI9341_BLACK);
//  delay(300);
//  drawCross(320 - 10, 240 - 10);
//  while (touchHeld(500));
//  while (!touchHeld(500));
// // eepromData.touchMAX = ts.getPoint();
//  tft.fillScreen(ILI9341_BLACK);
//
//  //drawCross(320-10,10);
//  //drawCross(320-10,240-10);
//  //drawCross(10,240-10);
//}



