/*********************************************************************
This is an example for our Monochrome OLEDs based on SSD1306 drivers
  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98
This example is for a 128x64 size display using I2C to communicate
3 pins are required to interface (2 I2C and one reset)
Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!
Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
Modified by Dan Peirce B.Sc.
The code now works like a terminal.
* text is being sent from Serial Monitor
* Intent is to have it sent from another MCU
* This tested on Arduino Uno
* Intent is to use Adafruit 5V Pro Trinket
*********************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSerif9pt7b.h>

#define OLED_RESET 4
#define W1L1X1 5
#define W1L1Y1 0
#define W1L1X2 95
#define W1L1Y2 0

#define W1L2X1 5
#define W1L2Y1 42
#define W1L2X2 95
#define W1L2Y2 42
//   display.drawLine(5, 0, 95, 0, WHITE);
//   display.drawLine(5, 42, 95, 42, WHITE); 


void defaultState();
void shiftoutS();
void xposS();
void yposS();
void xposL1S();
void yposL1S();
void xposL2S();
void yposL2S();
void fontS();
void tttoeS();
void tttoeDelS();
void windowS();

void showPHYS1600();
void showPhotogateTimer();
void showTictactoe();
void showLabels();
void deleteXO(uint16_t xpnt, uint16_t ypnt);
void window1(void);
void window2(void);

Adafruit_SSD1306 display(OLED_RESET);

// Insertion points for tic tac toe Xs nd Os
#define Xpnt1 22
#define Ypnt1 15
#define Xpnt2 52
#define Ypnt2 15
#define Xpnt3 79
#define Ypnt3 15
#define Xpnt4 22
#define Ypnt4 38
#define Xpnt5 52
#define Ypnt5 38
#define Xpnt6 79
#define Ypnt6 38
#define Xpnt7 22
#define Ypnt7 58
#define Xpnt8 52
#define Ypnt8 58
#define Xpnt9 79
#define Ypnt9 58

#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

void setup()   {                
  Serial.begin(115200);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for 3rd party 128x64)
  // init done
  
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(2000);

  // Clear the buffer.
  display.clearDisplay();

  showPhotogateTimer();

}

uint8_t incomingByte, count=1;

void (*statePnt)() = defaultState;

void loop() 
{
    statePnt(); // additional states will be added
}

void defaultState()
{
      // send data only when you receive data:
    if (Serial.available() > 0) 
    {
          // read the incoming byte:
          incomingByte = Serial.read();
          if ( incomingByte == 0x0C)   // using "FF" fast foreword ascii code
          {                            
            display.clearDisplay();
            display.display();
            display.setCursor(0,0);
          }
          else if ( incomingByte == 0x11)   // control device 1 -- size 1
          {                            
            display.setTextSize(1);
          }
          else if ( incomingByte == 0x12)   // control device 2 -- size 2
          {                            
            display.setTextSize(2);
          }
          else if ( incomingByte == 0x13)   //  device control 3 -- size 3
          {                            
            display.setTextSize(3);
          }
          else if ( incomingByte == 0x14)   // device control 4 -- size 4
          {                            
            display.setTextSize(4);
          }
          else if ( incomingByte == 0x09)   // Landscape Mode
          {                            
            display.setRotation(0);
          }
          else if ( incomingByte == 0x0B)   // Portrait Mode
          {                            
            display.setRotation(1);
          }
          else if ( incomingByte == 0x0E)   // Portrait Mode
          {                            
            statePnt = shiftoutS;
          }          
          else
          {
            display.write(incomingByte);
            if (incomingByte == '\n') display.display();
          }
    }

}

// Second level to menu added using SO (shift out ASCII character)
// This state is entered (transition from default state) if a shift out is recevied.
// So far this state looks for a back tick to indicate set position command
// It is likely that additional controls will be added
// if the command is not recognized then SO is canceled and control goes back to 
// default state.
void shiftoutS()
{
    if (Serial.available() > 0) 
    {
          // read the incoming byte:
          incomingByte = Serial.read();
          if ( incomingByte == '`')   // using back tick for set position
          {                            
            statePnt = xposS;
          }
          else if ( incomingByte == '-')
          {
            statePnt = xposL1S;  // using hyphen for draw line
          }
          else if ( incomingByte == 'f')
          {
            statePnt = fontS;  // using f for font
          }
          else if ( incomingByte == 'p' ) 
          {
            showPhotogateTimer();
            statePnt = defaultState;          
          }
          else if ( incomingByte == 't' ) statePnt = tttoeS;          
          else if ( incomingByte == 'r' ) 
          {
            display.clearDisplay();
            showPhotogateTimer();
            statePnt = defaultState;          
          }
          else if (incomingByte == 'w' ) statePnt = windowS;
          else statePnt = defaultState;
          
    }
}

void windowS()
{
    if (Serial.available() > 0) 
    {
          // read the incoming byte:
          incomingByte = Serial.read();
          if      ( incomingByte == '2') window2();             
          if      ( incomingByte == '1') window1();      
          statePnt = defaultState;
    }  
}
  
void window1(void)
{
  display.fillRect(W1L1X1, W1L1Y1+1, W1L1X2-W1L1X1+8, W1L2Y1-W1L1Y1-2, BLACK); // clear window1
  //display.display();
  display.setCursor(W1L2X1+1,W1L2Y1-7);
}

void window2(void)
{
  display.fillRect(W1L2X1, W1L2Y1+1, 96, 22, BLACK); // clear window2
  //display.display();
  display.setCursor(W1L2X1+1,57);
}

void tttoeS()
{
    if (Serial.available() > 0) 
    {
          statePnt = defaultState;
          // read the incoming byte:
          incomingByte = Serial.read();
          if      ( incomingByte == '1') display.setCursor(Xpnt1,Ypnt1);                            
          else if ( incomingByte == '2') display.setCursor(Xpnt2,Ypnt2);
          else if ( incomingByte == '3') display.setCursor(Xpnt3,Ypnt3);                            
          else if ( incomingByte == '4') display.setCursor(Xpnt4,Ypnt4);                            
          else if ( incomingByte == '5') display.setCursor(Xpnt5,Ypnt5);
          else if ( incomingByte == '6') display.setCursor(Xpnt6,Ypnt6);
          else if ( incomingByte == '7') display.setCursor(Xpnt7,Ypnt7);                            
          else if ( incomingByte == '8') display.setCursor(Xpnt8,Ypnt8);
          else if ( incomingByte == '9') display.setCursor(Xpnt9,Ypnt9);
          else if ( incomingByte == 's') showLabels();
          else if ( incomingByte == 'd') statePnt = tttoeDelS;
    }  
}

void tttoeDelS()
{ 
    if (Serial.available() > 0) 
    {
          statePnt = defaultState;
          // read the incoming byte:
          incomingByte = Serial.read();
          if      ( incomingByte == '1') deleteXO(Xpnt1, Ypnt1);
          else if ( incomingByte == '2') deleteXO(Xpnt2, Ypnt2);
          else if ( incomingByte == '3') deleteXO(Xpnt3, Ypnt3);                            
          else if ( incomingByte == '4') deleteXO(Xpnt4, Ypnt4);                           
          else if ( incomingByte == '5') deleteXO(Xpnt5, Ypnt5);
          else if ( incomingByte == '6') deleteXO(Xpnt6, Ypnt6);
          else if ( incomingByte == '7') deleteXO(Xpnt7, Ypnt7);                           
          else if ( incomingByte == '8') deleteXO(Xpnt8, Ypnt8);
          else if ( incomingByte == '9') deleteXO(Xpnt9, Ypnt9);
    }  
}

void deleteXO(uint16_t xpnt, uint16_t ypnt)
{
    int16_t  x1, y1;
    uint16_t w, h;
    display.getTextBounds(F("X"), xpnt, ypnt, &x1, &y1, &w, &h);
    display.fillRect(x1, y1-1, w, h+1, BLACK); // O is slightl taller than X
    display.display();
    display.setCursor(xpnt,ypnt);
}

void fontS()
{
    if (Serial.available() > 0) 
    {
          // read the incoming byte:
          incomingByte = Serial.read();
          if ( incomingByte == '0') display.setFont();                            
          else if ( incomingByte == '1') display.setFont(&FreeSerif9pt7b);
          statePnt = defaultState;
    }  
}

unsigned char xposV=0, xposV2;
unsigned char yposV=0, yposV2;

// xposL1S is entered if draw line was chosen after SO
// If the next byte received is 32 dec or more 32 dec will be subtracted
// the new value is taken as the x position

void xposL1S()
{
    if (Serial.available() > 0) 
    {
          // read the incoming byte:
          incomingByte = Serial.read();
          if ( incomingByte >= 0x20)   // 
          {
            xposV = incomingByte - 0x20;                            
          }
          else 
          {                            
            xposV = incomingByte;
          }
          statePnt = yposL1S;
    }  
}

// yposL1S is entered if set position was chosen after SO
// and an xposV has already been entered
// If the next byte received is 32 dec or more 32 dec will be subtracted
// the new value is taken as the y position
// once both positions are received the curser position will be updated

void yposL1S()
{
    if (Serial.available() > 0) 
    {
          // read the incoming byte:
          incomingByte = Serial.read();
          if ( incomingByte >= 0x20)   //
          {
            yposV = incomingByte - 0x20;                            
          }
          else 
          {                            
            yposV = incomingByte;
          }
          statePnt = xposL2S;
    }  
}

// If the next byte received is 32 dec or more 32 dec will be subtracted
// the new value is taken as the x position

void xposL2S()
{
    if (Serial.available() > 0) 
    {
          // read the incoming byte:
          incomingByte = Serial.read();
          if ( incomingByte >= 0x20)   // 
          {
            xposV2 = incomingByte - 0x20;                            
          }
          else 
          {                            
            xposV2 = incomingByte;
          }
          statePnt = yposL2S;
    }  
}

// yposL2S is entered if set position was chosen after SO
// and an xposV has already been entered
// If the next byte received is 32 dec or more 32 dec will be subtracted
// the new value is taken as the y position
// once both positions are received the curser position will be updated

void yposL2S()
{
    if (Serial.available() > 0) 
    {
          // read the incoming byte:
          incomingByte = Serial.read();
          if ( incomingByte >= 0x20)   //
          {
            yposV2 = incomingByte - 0x20;                            
          }
          else 
          {                            
            yposV2 = incomingByte;
          }
          display.drawLine(xposV, yposV, xposV2, yposV2,WHITE);
          display.display();
          statePnt = defaultState;
    }  
}

// xposS is entered if set position was chosen after SO
// If the next byte received is 32 dec or more 32 dec will be subtracted
// the new value is taken as the x position

void xposS()
{
    if (Serial.available() > 0) 
    {
          // read the incoming byte:
          incomingByte = Serial.read();
          if ( incomingByte >= 0x20)   // using back tick for set position
          {
            xposV = incomingByte - 0x20;                            
          }
          else 
          {                            
            xposV = incomingByte;
          }
          statePnt = yposS;
    }  
}


// yposS is entered if set position was chosen after SO
// and an zposV has already been entered
// If the next byte received is 32 dec or more 32 dec will be subtracted
// the new value is taken as the y position
// once both positions are received the curser position will be updated

void yposS()
{
    if (Serial.available() > 0) 
    {
          // read the incoming byte:
          incomingByte = Serial.read();
          if ( incomingByte >= 0x20)   // using back tick for set position
          {
            yposV = incomingByte - 0x20;                            
          }
          else 
          {                            
            yposV = incomingByte;
          }
          display.setCursor(xposV,yposV);
          statePnt = defaultState;
    }  
}

void showPHYS1600()
{
  display.setRotation(1);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(14,12);
  display.setFont(&FreeSerif9pt7b);
  display.println(F("KPU"));
  display.setRotation(0);
  display.setCursor(8,18);
  display.println(F("PHYS1600"));
  display.drawLine(W1L1X1, W1L1Y1, W1L1X2, W1L1Y2, WHITE);
  display.drawLine(W1L2X1, W1L2Y1, W1L2X2, W1L2Y2, WHITE);
  display.setCursor(20,35);
  display.println(F("  PMT"));  
  display.display();
}

void showTictactoe()
{
  display.setRotation(1);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(14,12);
  display.setFont(&FreeSerif9pt7b);
  display.println(F("KPU"));
  display.setRotation(0);
  display.drawLine(15, 21, 95, 21, WHITE);
  display.drawLine(15, 43, 95, 43, WHITE);
  display.drawLine(42, 0, 42, 63, WHITE);
  display.drawLine(71, 0, 71, 63, WHITE);
  display.setCursor(22,15);
  display.display();
}

void showLabels()
{
          display.setCursor(Xpnt1,Ypnt1); 
          display.print(1);                           
          display.setCursor(Xpnt2,Ypnt2);
          display.print(2);
          display.setCursor(Xpnt3,Ypnt3);                            
          display.print(3); 
          display.setCursor(Xpnt4,Ypnt4);                            
          display.print(4); 
          display.setCursor(Xpnt5,Ypnt5);
          display.print(5);
          display.setCursor(Xpnt6,Ypnt6);
          display.print(6);
          display.setCursor(Xpnt7,Ypnt7);                            
          display.println(7);
          display.setCursor(Xpnt8,Ypnt8);
          display.print(8);
          display.setCursor(Xpnt9,Ypnt9);
          display.print(9);
          display.display();
}
  
void showPhotogateTimer()
{
  display.setRotation(1);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(14,12);
  display.setFont(&FreeSerif9pt7b);
  display.println(F("KPU"));
  display.setRotation(0);
  display.setCursor(8,18);
  display.println(F("Photogate"));
  display.setCursor(20,35);
  display.println(F("Timer"));
  display.drawLine(W1L1X1, W1L1Y1, W1L1X2, W1L1Y2, WHITE);
  display.drawLine(W1L2X1, W1L2Y1, W1L2X2, W1L2Y2, WHITE); 
  display.display();
}

