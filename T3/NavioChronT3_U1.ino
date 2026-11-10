#include "Arduino.h"
#include "USB.h"
#include "USBHIDVendor.h"
#include "keycode.h"
#include <Adafruit_NeoPixel.h>
#include <driver/adc.h>
#include <Preferences.h>
#define ENCODER_DO_NOT_USE_INTERRUPTS
#include <Encoder.h>


#define ROWNUM 4
#define COLNUM 5
#define KC_NO -1
#define TESTONLY 0

#define KNOB1RATIO 2     // 中央旋钮
#define KNOB2RATIO 2     // 大转盘旋钮
#define WHEEL1RATIO 2    // 滚轮

int LEDPIN1 = 7; //1;
int VOLPIN = 3; //2;
int NUMPIXELS1 = 15;
int USBorBLE = 0, lowPowerMode = 0;
int batteryLevel = 100;
static int usb_suspended = 0;
static bool usb_previousstatus = false;

int LEDs[4][5] = {
  { -1, 3, 4, 5, -1 },
  { -1, -1, -1, 6, 7 },
  { -1, 11, 10, 9, 8 },
  { -1, 12, 13, 14, -1 }
};

int LEDExts[1][3] = {
  { 0, 1, 2 }
};

int MaxColor = 90;
int ColorBase = 0;
int ColorStep = 1;
int ColorState = MaxColor / ColorStep;
int LEDDelay = 2000 / ColorState;

unsigned long LEDLastMillis, LEDLastMillis2;
unsigned long lastUSBCheck = 0;
unsigned long lastSMDataMillis, SMDataMillis;
unsigned long lastKeyDataMicros, KeyDataMicros;
unsigned long curMillis, lastMillis;
unsigned long curBatteryMillis, lastBatteryMillis;
int batteryCheckInterval = 1000 * 15 * 1;  // Check battery every 5 minutes;

byte Cols[] = { 6, 5, 4, 2, 1 }; 
//byte Cols[] = { 15, 7, 6, 5, 4 };              // Outputs
byte Rows[] = { 13, 12, 11, 10 };
//byte Rows[] = { 11, 10, 9, 8 };  // Inputs
int CurrentLayer = 0;

int KeyMaps[5][ROWNUM][COLNUM] = {
  { { 0x00, 0x01, 0x02, 0x03, KC_NO },
    { 0x10, KC_NO, 0x12, 0x13, 0x14 },
    { KC_NO, 0x21, 0x22, 0x23, 0x24 },
    { 0x30, 0x31, 0x32, 0x33, KC_NO }},
  { { 0x01, 0x00, 0x02, 0x00, KC_NO },
    { 0x0a, KC_NO, 0x37, 0x22, 0x23 },
    { KC_NO, 0x2a, 0x10, 0x00, 0x03 },
    { 0x38, 0x12, 0x11, 0x13, KC_NO }},
  { { 0x01, 0x00, 0x02, 0x00, KC_NO },
    { 0x0a, KC_NO, 0x37, 0x22, 0x23 },
    { KC_NO, 0x2a, 0x10, 0x00, 0x03 },
    { 0x38, 0x12, 0x11, 0x13, KC_NO }},
  { { 0x01, 0x00, 0x02, 0x00, KC_NO },
    { 0x0a, KC_NO, 0x37, 0x22, 0x23 },
    { KC_NO, 0x2a, 0x10, 0x00, 0x03 },
    { 0x38, 0x12, 0x11, 0x13, KC_NO }},
  { { 0x01, 0x00, 0x02, 0x00, KC_NO },
    { 0x0a, KC_NO, 0x37, 0x22, 0x23 },
    { KC_NO, 0x2a, 0x10, 0x00, 0x03 },
    { 0x38, 0x12, 0x11, 0x13, KC_NO }}
};

int longPressDelay = 250;  //customizable keyboard values
int spamSpeed = 15;
int keyDown[ROWNUM][COLNUM];
bool keyLong[ROWNUM][COLNUM];
int bPressed[ROWNUM][COLNUM];



Adafruit_NeoPixel pixels1 = Adafruit_NeoPixel(NUMPIXELS1, LEDPIN1, NEO_GRB + NEO_KHZ800);
USBHIDVendor TVendor;
Encoder knob1(9, 8);
Encoder knob2(39, 40);
Encoder wheel1(15, 14);
//Encoder knob1(13, 12);
//Encoder knob2(17, 16);
//Encoder wheel1(46, 3);
long Pos1  = -999, Pos2 = -999, Pos3 = - 999;

void CheckBatteryLevel() {
  if (USBorBLE == 0) {
    // bleSpaceKat.batteryLevel = 100;
    return;
  }
  int Vbatt = analogReadMilliVolts(VOLPIN);
  // bleSpaceKat.batteryLevel = Vbatt * 2 * 100 / 1000 / 4;
  float tmpV = Vbatt * 2.0 / 1000.0;
  // Serial.printf("Battery Voltage: %fV", tmpV);
  tmpV = (tmpV - 3.1) / (4.19 - 3.1);
  if (tmpV <= 0)
    tmpV = 0;
  if (tmpV >= 1)
    tmpV = 1;
  // bleSpaceKat.batteryLevel = round(tmpV * 100);
  batteryLevel = round (tmpV * 100);
  // Serial.printf("   Battery Percentage: %d%", batteryLevel);
  // Serial.println();
  // if (USBorBLE == 1)
    // bleSpaceKat.setBatteryLevel(bleSpaceKat.batteryLevel);
}

void usbEventCallback(void *arg, esp_event_base_t event_base, 
                       int32_t event_id, void *event_data);

void setup() {
  Serial.begin(115200);
  for (int i=0; i<=ROWNUM-1; i++)
  {
    pinMode(Rows[i], OUTPUT);
    digitalWrite(Rows[i], HIGH);
  }
  for (int i=0; i<=COLNUM-1; i++)
  {
    pinMode(Cols[i], INPUT_PULLUP);
  }
  pixels1.begin();
  for (int i = 0; i <= NUMPIXELS1 - 1; i++) {
    pixels1.setPixelColor(i, 0, 64, 0);
  }
  pixels1.show();

  pinMode(VOLPIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(VOLPIN, ADC_11db);

  Pos1 = knob1.read() / KNOB1RATIO;
  Pos2 = knob2.read() / KNOB2RATIO;
  Pos3 = wheel1.read() / WHEEL1RATIO;


  TVendor.begin();
  USB.VID(0xCAFE);
  USB.PID(0xBEEF);
  USB.onEvent(usbEventCallback);
  #if (TESTONLY != 1)
    USB.begin();
  #endif
  CheckBatteryLevel();
  curBatteryMillis = millis();
  lastBatteryMillis = curBatteryMillis;
  LEDLastMillis = millis();
  LEDLastMillis2 = millis();

  curMillis = millis();
  lastMillis = curMillis;
  SMDataMillis = millis();
  lastSMDataMillis = millis();
  KeyDataMicros = micros();
  lastKeyDataMicros = micros();
  delay(1000);
  for (int i = 0; i <= NUMPIXELS1 - 1; i++) {
    pixels1.setPixelColor(i, 0, 0, 64);
  }
  pixels1.show();
}

void loop() {
  // return;
  KeyDataMicros = micros();
  if (KeyDataMicros < lastKeyDataMicros)
    lastKeyDataMicros = KeyDataMicros;
  
  if (KeyDataMicros - lastKeyDataMicros >= 2000 && !lowPowerMode) {
    if (1==1) {
      for (int i=0; i<=ROWNUM-1; i++)
      {
        digitalWrite(Rows[i], LOW);
        delayMicroseconds(5);

        for (int j=0; j<=COLNUM-1; j++)
        {
          if (Cols[j] >= 0 && digitalRead(Cols[j]) == LOW)
          {
            keyPressed(i,j); 

          }
          else if (keyDown[i][j] != 0)
          {
            resetKey(i,j);
          }
        }
        digitalWrite(Rows[i], HIGH);
        delayMicroseconds(5);
      }
    }
    
    long NewPos1 = knob1.read() / KNOB1RATIO;
    long NewPos2 = knob2.read() / KNOB2RATIO;
    long NewPos3 = wheel1.read() / WHEEL1RATIO;
    uint8_t encode = 0;
    if (NewPos1 != Pos1) {
      if (NewPos1 > Pos1)
        encode = 0x44;
      else
        encode = 0x04;
      Pos1 = NewPos1;
      TVendor.write(encode);
      TVendor.flush();
      // Serial.println(newPosition);
    }
    if (NewPos2 != Pos2) {
      if (NewPos2 > Pos2)
        encode = 0x4f;
      else
        encode = 0x0f;
      Pos2 = NewPos2;
      TVendor.write(encode);
      TVendor.flush();
      // Serial.println(newPosition);
    }
    if (NewPos3 != Pos3) {
      if (NewPos3 > Pos3)
        encode = 0x49;
      else
        encode = 0x09;
      Pos3 = NewPos3;
      TVendor.write(encode);
      TVendor.flush();
      // Serial.println(newPosition);
    }
  }
  
  //if (!tud_mounted()) {
    // esp_restart();
  //}

  //if (usb_previousstatus != usb_suspended)
  {
    usb_previousstatus = usb_suspended;
    for (int i = 0; i <= NUMPIXELS1 - 1; i++) {
      if (usb_suspended == 1)
        pixels1.setPixelColor(i, 0, 64, 64);
      else if (usb_suspended == 0)
        pixels1.setPixelColor(i, 64, 0, 64);
      else if (usb_suspended == 2)
        pixels1.setPixelColor(i, 32, 0, 64);
      else if (usb_suspended == 3)
        pixels1.setPixelColor(i, 64, 64, 0);
      else if (usb_suspended == 4)
        pixels1.setPixelColor(i, 32, 64, 64);
    }
    pixels1.show();
  }
}

void keyPressed(int row, int col){
/*
  Serial.print("Output: "); 
  Serial.print(row);
  Serial.print(" Input: ");
  Serial.print(col);
  Serial.print(" ");
  Serial.println(Keys[row][col]);
*/
  
  if(keyDown[row][col]==0){         //if the function is called for the first time for this key
    // Keyboard.write(Keys[row][col]);
    keyPressAction(KeyMaps[CurrentLayer][row][col]);
    // Serial.println(keyDown[row][col]);
    keyDown[row][col]++;
    int ledIndex = LEDs[row][col];
    pixels1.setPixelColor(ledIndex, 0, 0, 64);
    pixels1.show();

  }
  

  
/*
  Serial.print(keyLong[row][col]);
  Serial.print(", ");
  Serial.println(keyDown[row][col]);
*/
}

void resetKey(int row, int col){ //resetting the variables after key is released
  keyDown[row][col] = 0;
  keyLong[row][col] = false;
  int ledIndex = LEDs[row][col];
  pixels1.setPixelColor(ledIndex, 0, 64, 0);
  pixels1.show();
  keyResetAction(KeyMaps[CurrentLayer][row][col]);
}

void keyPressAction(int KeyCode)
{
  uint8_t press;
  #if (TESTONLY == 1)
    Serial.println(KeyCode);
  #endif
    
  if (KeyCode >= 0)
  {
    press = KeyCode;
    TVendor.write(press);
    TVendor.flush();
  }
}

void keyResetAction(int KeyCode)
{
  uint8_t release;
  if (KeyCode >= 0)
  {
    release = KeyCode | 0x80;
    TVendor.write(release);
    TVendor.flush();
  }      
}


void tud_suspend_cb(bool remote_wakeup_en) {
    usb_suspended = 1;
    // 如果你希望在此处执行断开操作，也可以直接调用 USB.end()
    // 但通常设置一个标志位，让主循环来处理更安全
    // Serial.println("[USB] Bus Suspended (PC Sleep detected)");
}

// 当USB总线从挂起状态恢复（电脑唤醒）时被TinyUSB调用
void tud_resume_cb(void) {
    usb_suspended = 2;
    // 关键：在这里设置一个标志位，通知主循环USB总线已恢复
    Serial.println("[USB] Bus Resumed (PC Wake-up detected)");
}

void usbEventCallback(void *arg, esp_event_base_t event_base, 
                       int32_t event_id, void *event_data) {
    switch (event_id) {
        case ARDUINO_USB_SUSPEND_EVENT:
            // 电脑进入睡眠/挂起
            Serial.println("[USB] SUSPEND event - PC sleeping");
            usb_suspended = 1;
            // 可以在这里停止发送数据、进入低功耗模式等
            break;
            
        case ARDUINO_USB_RESUME_EVENT:
            // 电脑唤醒，USB总线恢复
            Serial.println("[USB] RESUME event - PC woke up");
            usb_suspended = 2;
            // 关键：检测串口是否可用，若不可用则强制重连
            
            break;
            
        case ARDUINO_USB_STARTED_EVENT:
            usb_suspended = 3;
            Serial.println("[USB] STARTED event");
            break;
            
        case ARDUINO_USB_STOPPED_EVENT:
            usb_suspended = 4;
            Serial.println("[USB] STOPPED event");
            break;
            
        default:
            break;
    }
}
