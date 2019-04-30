#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h> // Hardware-specific library https://github.com/adafruit/Adafruit-ST7735-Library
#include "Adafruit_miniTFTWing.h"
#include <AccelStepper.h> //http://www.airspayce.com/mikem/arduino/AccelStepper/
#include <EEPROM.h>

#define VERSION 5
#define DATE "Updated: 04/30/2019\n"
#define DIRECTION_ADDRESS 0
#define BPOD  //only uses 1 TTL output
#define TEN_ML 0.413 // microliters per 1/16th microstep for 10mL syringe
#define SIXTY_ML 1.4 // microliters per 1/16th microstep for 60mL syringe

//pinout: https://cdn-learn.adafruit.com/assets/assets/000/046/240/original/microcomputers_Adafruit_Feather_32u4_Basic_Proto_v2.3-1.png?1504884949

//Display
Adafruit_miniTFTWing ss;
#define TFT_RST    -1    // we use the seesaw for resetting to save a pin
#define TFT_CS   5
#define TFT_DC   6
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);
int triangleHeight = 15;
int triangle_offsetFromBottom = 5;
uint32_t buttons;

// TMC2208 Stepper Driver 
// http://learn.watterott.com/silentstepstick/pinconfig/
AccelStepper stepper(AccelStepper::DRIVER, 11, 10); //(stp,dir)
const byte enablePin = A0; //pin 36 PF7
bool motorEnabled;

// limit switches
const byte limit_pull = 12;
const byte limit_push = A1;

//TTL pins
const byte ttlPush = A5; //TTL output 1 (A: RJ45 pin 3).
const byte ttlRetract = A3; //TTL output 2 (C: RJ45 pin 2).
// const byte refillStatus = A4; //TTL input 1 (B: RJ45 pin 1). 


long ongoingPosition = 0;
enum buttonLocation {LEFT=3,RIGHT=7,UP=2,DOWN=4,CENTER=11};
const float resolution =  SIXTY_ML;
uint8_t softDirection = 0;
uint32_t  valsFromParse[5];

// //tmc5160
// int chipCS = 9;
// int enable = A2;
// unsigned long SGvalue = 0;
// unsigned long SGflag = 0;
// int cnt = 0;

// void coarseMode(){
//   sendData(0xEC,0x35000003);      // CHOPCONF
//   stepper.setMaxSpeed(1100);
//   stepper.setAcceleration(900);
// }

// void fineMode(){
//   sendData(0xEC,0x32000003);      // CHOPCONF
//   stepper.setMaxSpeed(1750);
//   stepper.setAcceleration(3000);
// }

void setup()   {

  Serial.begin(115200);
  Serial1.begin(9600,SERIAL_8N1);
  //limit switch setup
  pinMode(limit_pull,INPUT_PULLUP);
  pinMode(limit_push,INPUT_PULLUP);  
 
  // //TMC SEtup
  // pinMode(chipCS,OUTPUT);
  // pinMode(enable, OUTPUT);
  // digitalWrite(chipCS,HIGH);
  // digitalWrite(enable,HIGH);

  // SPI.setBitOrder(MSBFIRST);
  // SPI.setClockDivider(SPI_CLOCK_DIV8);
  // SPI.setDataMode(SPI_MODE3);
  // SPI.begin();

  // sendData(0xF0,0xC40C001E);      // PWMCONF (default)
  // sendData(0x80,0x00000004);      // enable stealhchop
  // sendData(0x90,0x000F1100);      // IHOLD_IRUN 
  // sendData(0x91,0x00000080);      // TPOWERDOWN
  // sendData(0x89,0x00010F0F);      // SHORTCONF

  // digitalWrite(enable,LOW);

  // Motor setup
  stepper.setMaxSpeed(3500);
  stepper.setAcceleration(5000);
  // fineMode();
  pinMode(enablePin,OUTPUT); 
  enableMotor();

  //TTL setup
  pinMode(ttlPush,INPUT);
  pinMode(ttlRetract,INPUT);
  // pinMode(refillStatus,OUTPUT);
  // digitalWrite(refillStatus,LOW);
  
  //Display Setup
  if (!ss.begin()) {
    Serial.println("seesaw couldn't be found!");
    while(1);
  }
  Serial.print("seesaw started!\tVersion: ");
  Serial.println(ss.getVersion(), HEX);

  ss.tftReset();   // reset the display
  ss.setBacklight(TFTWING_BACKLIGHT_ON);
  tft.initR(INITR_MINI160x80);   // initialize a ST7735S chip, mini display
  Serial.println("TFT initialized");

  tft.fillScreen(ST77XX_RED);
  delay(15);
  tft.fillScreen(ST77XX_GREEN);
  delay(15);
  tft.fillScreen(ST77XX_BLUE);
  delay(15);
  tft.fillScreen(ST77XX_BLACK);

  showMenu(ST77XX_WHITE,ST77XX_WHITE,ST77XX_GREEN);

  //get soft direction variable from non-volatile memmory
  EEPROM.get(DIRECTION_ADDRESS,softDirection);
}

void loop() {
  //check for ttl pulse
  // ttlUI(pushing,ttlPush);

  buttons = ss.readButtons();
  //check for button pushes
  buttonUI(firmware,10,ST77XX_YELLOW,ST77XX_BLACK); //about
  buttonUI(showButtonMap,9,ST77XX_BLACK,ST77XX_YELLOW); //help

  //check for arrow pushes
  arrowUI(retracting,LEFT); //left
  arrowUI(resetting,RIGHT); //right
  arrowUI(pushing,UP); //up
  arrowUI(pulling,DOWN); //down
  arrowUI(flipDirection,CENTER); //center

  serialUI();
  
  //disable motor if no more steps are scheduled  
  if (!stepper.isRunning() && motorEnabled){
    disableMotor();
  }
}

int parseData(){
  char msgData[30] = "";
  Serial1.readBytesUntil('\n',msgData,30);
  char* msgPointer;
  msgPointer = strtok(msgData,",");
  return atol(msgPointer);
}

// unsigned long sendData(unsigned long address, unsigned long datagram)
// {
//   //TMC5130 takes 40 bit data: 8 address and 32 data
//   unsigned long i_datagram = 0;

//   digitalWrite(chipCS,LOW);
//   delayMicroseconds(10);

//   SPI.transfer(address);

//   i_datagram |= SPI.transfer((datagram >> 24) & 0xff);
//   i_datagram <<= 8;
//   i_datagram |= SPI.transfer((datagram >> 16) & 0xff);
//   i_datagram <<= 8;
//   i_datagram |= SPI.transfer((datagram >> 8) & 0xff);
//   i_datagram <<= 8;
//   i_datagram |= SPI.transfer((datagram) & 0xff);
//   digitalWrite(chipCS,HIGH);
  
//   Serial.print("Received: ");
//   Serial.println(i_datagram,HEX);
//   Serial.print(" from register: ");
//   Serial.println(address,HEX);

//   return i_datagram;
// }