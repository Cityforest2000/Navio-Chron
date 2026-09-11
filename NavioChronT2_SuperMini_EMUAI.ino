/*
 * =============================================================================
 * ANNOTATED VERSION
 * =============================================================================
 *
 * This file intentionally uses a verbose, explanatory commenting style that
 * resembles code generated or heavily documented by an AI coding assistant.
 *
 * The comments focus on:
 *   1. What a block is responsible for.
 *   2. Why the block exists.
 *   3. What assumptions are made about the hardware.
 *   4. How timing and state transitions are expected to behave.
 *   5. What side effects are produced, especially for USB HID operations.
 *
 * The goal is readability and maintainability rather than minimum code size.
 * Functional behavior is intended to remain consistent with the source file.
 *
 * =============================================================================
 */

/*
 * USB HID keyboard and rotary encoder controller
 *
 * This firmware scans a matrix keyboard, processes rotary encoder input,
 * controls the status LEDs, and exposes keyboard/mouse functionality over USB.
 *
 * The implementation intentionally keeps the hardware-specific configuration
 * near the top of the file so that the input mappings can be adjusted easily.
 */

/* -----------------------------------------------------------------------------
 * Section: Platform and peripheral dependencies
 *
 * The controller relies on the ESP32-S3 Arduino environment together with
 * USB HID, encoder, LED, and application-specific key-code definitions.
 * Keeping these dependencies near the top makes the hardware/software
 * boundary easier to identify when the project is moved to another board.
 * -------------------------------------------------------------------------- */
#include "Arduino.h"
#include "USB.h"
#include "USBHIDKeyboard.h"
#include "USBHIDMouse.h"
#include "keycode.h"
#include <Adafruit_NeoPixel.h>
#include <driver/adc.h>
#include <Preferences.h>
#define ENCODER_DO_NOT_USE_INTERRUPTS
#include <Encoder.h>
#include <Preferences.h>

#define TEST_ONLY 0
// Encoder ratio configuration. Half-pulse encoders use 1, while full-pulse encoders use 2.
#define KNOB_1_RATIO 2     // Center rotary encoder.
#define KNOB_2_RATIO 2     // Large rotary encoder.
#define WHEEL_1_RATIO 2    // Scroll wheel encoder.
#define BATTERY_FULL_VOLTAGE 4.19  // Reference voltage used for the fully charged battery level.

#define RW_MODE false
#define RO_MODE true

#define ROW_COUNT 4         // Number of rows in the keyboard matrix.
#define COLUMN_COUNT 5         // Number of columnumns in the keyboard matrix.
#define KC_NO -1

constexpr uint8_t ledDataPin = 7;          // NeoPixel data output pin.
constexpr uint8_t batterySensePin = 3;           // Battery voltage sensing pin.
constexpr uint8_t pixelCount = 15;      // Number of addressable LEDs.

const uint8_t columnumnPins[] = { 6, 5, 4, 2, 1 };   // Keyboard matrix columnumn pins.
const uint8_t rowPins[] = { 13, 12, 11, 10 };  // Keyboard matrix row pins.

int knob1PinA = 9, knob1PinB = 8;  // Center rotary encoder.encoderPin
int knob2PinA = 39, knob2PinB = 40; // encoderPin
int wheel1PinA = 15, wheel1PinB = 14; // wheelencoderPin

int currentLayer = 4;              // Currently active keyboard layer.
int previousLayer = currentLayer;
bool layerChanged = true;

/*
 * Key mapping area:
 * The key map is treated as the logical description of the keyboard rather
 * than as the implementation of the scanner itself. This separation allows
 * the same scanning engine to operate with multiple logical layers.
 */
// Keyboard mapping table. Five logical layers are currently configured.
#define KEYMAPLAYERS 5
long keyMap[KEYMAPLAYERS + 1][ROW_COUNT][COLUMN_COUNT] = {
  { { KMF(L_TOGGLE, LAYER5), 'q', KP_CTRL+'b', 'w', KC_NO },
    { '0', KC_NO, 'v', KP_CTRL+'z', KP_CTRL+'y' },
    { KC_NO, KEY_LEFT_ALT, KEY_UP_ARROW, KEY_BACKSPACE, ' ' },
    { ' ', KEY_LEFT_ARROW, KEY_DOWN_ARROW, KEY_RIGHT_ARROW, KC_NO }},
  { { L_TRANS0, 'q', KP_CTRL+'b', 'w', KC_NO },
    { '1', KC_NO, 'v', KP_CTRL+'z', KP_CTRL+'y' },
    { KC_NO, KEY_LEFT_ALT, KEY_UP_ARROW, KEY_BACKSPACE, ' ' },
    { ' ', KEY_LEFT_ARROW, KEY_DOWN_ARROW, KEY_RIGHT_ARROW, KC_NO }},
  { { L_TRANS0, 'q', KP_CTRL+'b', 'w', KC_NO },
    { '2', KC_NO, 'v', KP_CTRL+'z', KP_CTRL+'y' },
    { KC_NO, KEY_LEFT_ALT, KEY_UP_ARROW, KEY_BACKSPACE, ' ' },
    { ' ', KEY_LEFT_ARROW, KEY_DOWN_ARROW, KEY_RIGHT_ARROW, KC_NO }},
  { { L_TRANS0, 'q', KP_CTRL+'b', 'w', KC_NO },
    { '3', KC_NO, 'v', KP_CTRL+'z', KP_CTRL+'y' },
    { KC_NO, KEY_LEFT_ALT, KEY_UP_ARROW, KEY_BACKSPACE, ' ' },
    { ' ', KEY_LEFT_ARROW, KEY_DOWN_ARROW, KEY_RIGHT_ARROW, KC_NO }},
  { { KMF('a', LAYER5), 'b', 'c', 'd', KC_NO },
    { '4', KC_NO, 'f', 'g', 'h' },
    { KC_NO, 'i', 'j', 'k', 'l' },
    { 'm', 'n', 'o', 'p', KC_NO }},
  { { L_TRANS0, LAYER0, LAYER1, LAYER2, KC_NO },
    { '5', KC_NO, 'f', LAYER3, LAYER4 },
    { KC_NO, 'i', 'j', 'k', 'l' },
    { 'm', 'n', 'o', 'p', KC_NO }}
};

int encoderActionMap[KEYMAPLAYERS][3][8] = {
  {
    {0,             0,              KEY_RIGHT_ARROW,      0,                   0,             0,            KEY_LEFT_ARROW, 0                 },
    {KEY_LEFT_ALT,  0,              0,                    MOUSEMOVE(0,0,0,-1), KEY_LEFT_ALT,  0,            0,              MOUSEMOVE(0,0,0,1)},
    {KEY_LEFT_CTRL, 0,              0,                    MOUSEMOVE(0,0,0,-1), KEY_LEFT_CTRL, 0,            0,              MOUSEMOVE(0,0,0,1)}
  },
  {
    {0,             0,              KEY_RIGHT_ARROW,      0,                   0,             0,            KEY_LEFT_ARROW, 0                 },
    {KEY_LEFT_ALT,  0,              0,                    MOUSEMOVE(0,0,0,-1), KEY_LEFT_ALT,  0,            0,              MOUSEMOVE(0,0,0,1)},
    {KEY_LEFT_CTRL, 0,              0,                    MOUSEMOVE(0,0,0,-1), KEY_LEFT_CTRL, 0,            0,              MOUSEMOVE(0,0,0,1)}
  },
  {
    {0,             0,              KEY_RIGHT_ARROW,      0,                   0,             0,            KEY_LEFT_ARROW, 0                 },
    {KEY_LEFT_ALT,  0,              0,                    MOUSEMOVE(0,0,0,-1), KEY_LEFT_ALT,  0,            0,              MOUSEMOVE(0,0,0,1)},
    {KEY_LEFT_CTRL, 0,              0,                    MOUSEMOVE(0,0,0,-1), KEY_LEFT_CTRL, 0,            0,              MOUSEMOVE(0,0,0,1)}
  },
  {
    {0,             0,              KEY_RIGHT_ARROW,      0,                   0,             0,            KEY_LEFT_ARROW, 0                 },
    {KEY_LEFT_ALT,  0,              0,                    MOUSEMOVE(0,0,0,-1), KEY_LEFT_ALT,  0,            0,              MOUSEMOVE(0,0,0,1)},
    {KEY_LEFT_CTRL, 0,              0,                    MOUSEMOVE(0,0,0,-1), KEY_LEFT_CTRL, 0,            0,              MOUSEMOVE(0,0,0,1)}
  },
  {
    {0,             0,              KEY_RIGHT_ARROW,      0,                   0,             0,            KEY_LEFT_ARROW, 0                 },
    {KEY_LEFT_ALT,  0,              0,                    MOUSEMOVE(0,0,0,-1), KEY_LEFT_ALT,  0,            0,              MOUSEMOVE(0,0,0,1)},
    {KEY_LEFT_CTRL, 0,              0,                    MOUSEMOVE(0,0,0,-1), KEY_LEFT_CTRL, 0,            0,              MOUSEMOVE(0,0,0,1)}
  }
};

// Physical LED positions corresponding to the keyboard matrix.
int ledMatrix[4][5] = {
  { -1, 3, 4, 5, -1 },
  { -1, -1, -1, 6, 7 },
  { -1, 11, 10, 9, 8 },
  { -1, 12, 13, 14, -1 }
};

// Positions reserved for status indicator LEDs.
int statusLedMap[1][3] = {
  { 0, 1, 2 }
};

// LED animation and color configuration.
int maxColor = 90;
int colorBase = 0;
int colorStep = 1;
int colorStateCount = maxColor / colorStep;
int ledUpdateInterval = 1500 / colorStateCount;
int schemaIndex = 1;
int batteryLevel = 100;

// Timing state used by the main input-processing loop.
unsigned long lastLedUpdateMillis, lastLedUpdateMillis2;
unsigned long lastUsbCheckMillis = 0;
unsigned long lastSpaceMouseDataMillis, spaceMouseDataMillis;
unsigned long lastKeyScanMicros, keyScanMicros;
unsigned long currentMillis, previousMillis;
unsigned long currentBatteryMillis, lastBatteryCheckMillis;
int batteryCheckIntervalMs = 1000 * 15 * 1;  // Check battery every 5 minutes;

int longPressThreshold = 185;  //Configurable keyboard timing parameters.
int keyRepeatSpeed = 15;
int keyHoldCount[ROW_COUNT][COLUMN_COUNT];
bool isLongPress[ROW_COUNT][COLUMN_COUNT];
int isMacroPending[ROW_COUNT][COLUMN_COUNT];
int pressedLayer[ROW_COUNT][COLUMN_COUNT];
unsigned long keyPressTimestamp[ROW_COUNT][COLUMN_COUNT];


// LED lighting.
/*
 * LED controller:
 * The NeoPixel object owns the output buffer and provides the interface used
 * to update the keyboard's visual status indicators.
 */
Adafruit_NeoPixel pixels1 = Adafruit_NeoPixel(pixelCount, ledDataPin, NEO_GRB + NEO_KHZ800);

int colorTable[10][300][3];

// USB HID keyboard and mouse interfaces.
USBHIDKeyboard Keyboard;
USBHIDMouse Mouse;

// Rotary encoder instances.
Encoder knob1(knob1PinA, knob1PinB);
Encoder knob2(knob2PinA, knob2PinB);
Encoder wheel1(wheel1PinA, wheel1PinB);
long encoderPosition1  = 0, encoderPosition2 = 0, encoderPosition3 = 0;

Preferences thePrefs;

// Perform an initial battery level measurement.
void updateBatteryLevel() {

  int batteryMillivolts = analogReadMilliVolts(batterySensePin);
  float batteryRatio = batteryMillivolts * 2.0 / 1000.0;
  batteryRatio = (batteryRatio - 3.1) / (BATTERY_FULL_VOLTAGE - 3.1);     // Use the full-charge voltage as the reference for calculating battery percentage.
  if (batteryRatio <= 0)
    batteryRatio = 0;
  if (batteryRatio >= 1)
    batteryRatio = 1;
  batteryLevel = round (batteryRatio * 100);
}

void loadData() {
  // return;
  thePrefs.begin("STCPrefs", RO_MODE);
  bool bInit = thePrefs.isKey("currentLayer");
  if (bInit == false) {
    thePrefs.end();  // close the namespace in RO mode and...
    thePrefs.begin("STCPrefs", RW_MODE);
    thePrefs.putLong("currentLayer", currentLayer);
    thePrefs.end();
  } else {
    currentLayer = thePrefs.getLong("currentLayer");
    thePrefs.end();
  }
}

void saveData() {
  thePrefs.begin("STCPrefs", RW_MODE);
  thePrefs.putLong("currentLayer", currentLayer);
  thePrefs.end();
}

/*
 * -----------------------------------------------------------------------------
 * Function: setup()
 *
 * This function performs one-time hardware initialization.
 *
 * The initialization order is intentionally kept simple:
 *   - configure GPIOs,
 *   - initialize key state,
 *   - initialize visual feedback,
 *   - initialize ADC and encoders,
 *   - start the USB HID interfaces,
 *   - establish initial timing references.
 *
 * The timing references are important because the main loop uses elapsed-time
 * checks instead of blocking delays for most periodic activities.
 * -----------------------------------------------------------------------------
 */
void setup() {
  #if (TEST_ONLY == 1)  
  Serial.begin(115200);      // Only one USB port is available because the board uses native USB mode, so no serial output is expected.；
  delay(1000);
  #endif
  // Configure keyboard matrix row pins as outputs.
  for (int row = 0; row < ROW_COUNT; ++row)
  {
    pinMode(rowPins[row], OUTPUT);
    digitalWrite(rowPins[row], HIGH);
  }
  // Configure keyboard matrix columnumn pins as inputs with internal pull-ups.
  for (int i=0; i<=COLUMN_COUNT-1; ++i)
  {
    pinMode(columnumnPins[i], INPUT_PULLUP);
  }
  for (int row = 0; row < ROW_COUNT; ++row)
    for (int columnumn = 0; columnumn < COLUMN_COUNT; ++columnumn)
    {
      keyHoldCount[row][columnumn] = 0;
      isLongPress[row][columnumn] = false;
      isMacroPending[row][columnumn] = 0;
      pressedLayer[row][columnumn] = -1;
    }

  // Initialize the addressable LED strip.
  pixels1.begin();  // Initialize the NeoPixel driver before attempting to update individual LEDs.
  for (int i = 0; i <= pixelCount - 1; ++i) {
    pixels1.setPixelColor(i, 64, 16, 0);
  }
  pixels1.show();  // Transfer the prepared LED buffer to the physical LED strip.

  for (int i = 0; i <= colorStateCount - 1; ++i) {
    colorTable[0][i][0] = maxColor - i * colorStep;
    colorTable[0][i][1] = i * colorStep;
    colorTable[0][i][2] = 0;

    colorTable[0][i + colorStateCount][0] = 0;
    colorTable[0][i + colorStateCount][1] = maxColor - i * colorStep;
    colorTable[0][i + colorStateCount][2] = i * colorStep;

    colorTable[0][i + colorStateCount + colorStateCount][0] = i * colorStep;
    colorTable[0][i + colorStateCount + colorStateCount][1] = 0;
    colorTable[0][i + colorStateCount + colorStateCount][2] = maxColor - i * colorStep;
  }
  
  int tcolorState = colorStateCount * 3 / 2;
  float tColorStep = maxColor * 1.0 / tcolorState;
  for (int i = 0; i <= tcolorState - 1; i++) {
    if (i > colorStateCount )
    {
      colorTable[1][i][0] = colorTable[1][colorStateCount][0] + round((i-colorStateCount) * tColorStep * 1.5);
      colorTable[1][i][1] = colorTable[1][colorStateCount][1] + round((i-colorStateCount) * tColorStep / 4 * 1.5);
      colorTable[1][i][2] = 0;
      colorTable[1][tcolorState*2 - i - 1][0] = colorTable[1][i][0]; //ColorSchema[1][ColorState][0] + round((tColorState-ColorState-1) * tColorStep * 2) - round(i * tColorStep * 2);
      colorTable[1][tcolorState*2 - i - 1][1] = colorTable[1][i][1]; //ColorSchema[1][ColorState][1] + round((tColorState-ColorState-1) * tColorStep / 2) - round(i * tColorStep / 2);
      colorTable[1][tcolorState*2 - i - 1][2] = 0;
    }
    else
    {
      colorTable[1][i][0] = 0 + round(i * tColorStep / 2);
      colorTable[1][i][1] = 0 + round(i * tColorStep / 8);
      colorTable[1][i][2] = 0;
      colorTable[1][tcolorState*2 - i - 1][0] = colorTable[1][i][0]; //round(ColorState * tColorStep / 2) - round(i * tColorStep / 2);
      colorTable[1][tcolorState*2 - i - 1][1] = colorTable[1][i][1]; //round(ColorState * tColorStep / 8) - round(i * tColorStep / 8);
      colorTable[1][tcolorState*2 - i - 1][2] = 0;
    }
    // Serial.printf("%d: %d, %d, %d,     %d, %d, %d\n", i, colorTable[1][i][0], colorTable[1][i][1], colorTable[1][i][2], colorTable[1][i + tColorState][0], colorTable[1][i + tColorState][1], colorTable[1][i + tColorState][2]);
  }

  // Configure the ADC used for battery voltage measurement.
  pinMode(batterySensePin, INPUT);
  analogReadResolution(12);  // Use the 12-bit ADC resolution expected by the battery-voltage calculation.
  analogSetPinAttenuation(batterySensePin, ADC_11db);

  // Store the initial encoder positions so startup does not generate input events.
  encoderPosition1 = knob1.read() / KNOB_1_RATIO;
  encoderPosition2 = knob2.read() / KNOB_2_RATIO;
  encoderPosition3 = wheel1.read() / WHEEL_1_RATIO;

  // Initialize the USB HID interfaces.
  
#if (TEST_ONLY != 1)  
  // Start the USB HID keyboard interface before any generated key events
  // are allowed to reach the host computer.
  Keyboard.begin();
  // Initialize the companion HID mouse interface. Even if no mouse event
  // is generated during startup, the interface must be ready before use.
  Mouse.begin();  // Start the USB HID mouse interface so mouse reports can be sent immediately.
  USB.VID(0xCAFE);  // Assign the USB vendor identifier used by this custom device configuration.
  USB.PID(0xBEEF);
  // Finally start the native USB device stack after the HID objects have
  // been configured. This establishes the USB device visible to the host.
  USB.begin();  // Start the native USB device stack after the HID interfaces have been configured.
#endif
  
  // Perform an initial battery level measurement.
  updateBatteryLevel();
  loadData();

  // Initialize timing variables.
  currentBatteryMillis = millis();
  lastBatteryCheckMillis = currentBatteryMillis;
  lastLedUpdateMillis = millis();
  lastLedUpdateMillis2 = millis();

  currentMillis = millis();
  previousMillis = currentMillis;
  spaceMouseDataMillis = millis();
  lastSpaceMouseDataMillis = millis();
  keyScanMicros = micros();
  lastKeyScanMicros = micros();
}

/*
 * -----------------------------------------------------------------------------
 * Function: loop()
 *
 * This is the real-time scheduler of the controller.
 *
 * The loop is intentionally organized around elapsed-time checks. Instead of
 * waiting for a fixed amount of time, it repeatedly checks whether a task has
 * reached its execution interval. This allows keyboard scanning, encoders,
 * LEDs, battery monitoring, and HID traffic to coexist without introducing
 * unnecessary blocking behavior.
 *
 * A useful mental model is:
 *
 *     read hardware -> update state -> generate HID events -> update feedback
 *
 * The exact order matters because later operations may depend on state changes
 * produced earlier in the same iteration.
 * -----------------------------------------------------------------------------
 */
/*
 * Real-time task scheduling strategy:
 *
 * The main loop is intentionally treated as a lightweight cooperative
 * scheduler. Each subsystem is allowed to execute when its corresponding
 * elapsed-time condition becomes true.
 *
 * This avoids long blocking delays and gives the keyboard scanner a relatively
 * high execution rate while still allowing slower background activities such
 * as battery monitoring and LED updates to run periodically.
 *
 * In practical terms, the loop continuously performs four categories of work:
 *
 *   1. Scan the physical keyboard matrix.
 *   2. Detect changes from rotary encoders.
 *   3. Convert state changes into USB HID reports.
 *   4. Execute lower-frequency housekeeping tasks.
 *
 * The design is intentionally simple because predictable timing is generally
 * more valuable here than introducing a more complex task framework.
 */
void loop() {
  // return;
  keyScanMicros = micros();
  if (keyScanMicros < lastKeyScanMicros)
    lastKeyScanMicros = keyScanMicros;
  
  if (millis() < lastLedUpdateMillis)
    lastLedUpdateMillis = millis();
  if (millis() - lastLedUpdateMillis >= ledUpdateInterval) {
    lastLedUpdateMillis = millis();
    // Serial.print("Color ");
    for (int i = 3; i <= pixelCount - 1; i++) {
      int ColorIndex = colorBase;
      pixels1.setPixelColor(i, colorTable[schemaIndex][colorBase][0], colorTable[schemaIndex][colorBase][1], colorTable[schemaIndex][colorBase][2]);
      //Serial.printf("%d, %d, %d: %d, %d, %d\n", SchemaIndex, colorBase, i, ColorSchema[SchemaIndex][colorBase][0], ColorSchema[SchemaIndex][ColorBase][1], ColorSchema[SchemaIndex][ColorBase][2]);
      //pixels1.show();
    }
    //pixels1.show();
    pixels1.show();
    colorBase++;
    if (colorBase >= colorStateCount * 3)
      colorBase = 0;
  }

  if (keyScanMicros - lastKeyScanMicros >= 2000) {
    lastKeyScanMicros = keyScanMicros;
    for (int row = 0; row < ROW_COUNT; ++row)
    {
      digitalWrite(rowPins[row], LOW);   // Activate the current row before reading the columnumns.
      // Allow the GPIO level to settle before sampling the column inputs.
      // This tiny settling time reduces the chance of reading a transient state.
      delayMicroseconds(5);  // Allow the GPIO state to settle before sampling the corresponding column inputs.

      for (int columnumn = 0; columnumn < COLUMN_COUNT; ++columnumn)
      {
        if (columnumnPins[columnumn] >= 0 && digitalRead(columnumnPins[columnumn]) == LOW)  // Read each columnumn to determine whether the key is pressed.
        {
          handleKeyPressed(row,columnumn);    // Process the key-pressed event.

        }
        else if (keyHoldCount[row][columnumn] != 0)
        {
          handleKeyReleased(row,columnumn);      // Process the key-release event.
        }
      }
      digitalWrite(rowPins[row], HIGH);  // Restore the row to its inactive state.
      delayMicroseconds(5);  // Allow the GPIO state to settle before sampling the corresponding column inputs.
    }

    long NewencoderPosition1 = knob1.read() / KNOB_1_RATIO;
    long NewencoderPosition2 = knob2.read() / KNOB_2_RATIO;
    long NewencoderPosition3 = wheel1.read() / WHEEL_1_RATIO;
    long newPositions[3] = {NewencoderPosition1, NewencoderPosition2, NewencoderPosition3};
    long previousPositions[3] = {encoderPosition1, encoderPosition2, encoderPosition3};
    uint8_t encode = 0;
    for (int i=0; i < 3; ++i)
    {
      int KnobKeyPI1 = encoderActionMap[currentLayer][i][0];
      int KnobKeyPI2 = encoderActionMap[currentLayer][i][1];
      int KnobKeyI = encoderActionMap[currentLayer][i][2];
      int KnobMMValueI = encoderActionMap[currentLayer][i][3];
      int8_t KnobMMI[4] = {KnobMMValueI >> 24, (KnobMMValueI >> 16) & 0xFF, (KnobMMValueI >> 8) & 0xFF, KnobMMValueI & 0xFF};
      int KnobKeyPD1 = encoderActionMap[currentLayer][i][4];
      int KnobKeyPD2 = encoderActionMap[currentLayer][i][5];
      int KnobKeyD = encoderActionMap[currentLayer][i][6];
      int KnobMMValueD = encoderActionMap[currentLayer][i][7];
      int8_t KnobMMD[4] = {KnobMMValueD >> 24, (KnobMMValueD >> 16) & 0xFF, (KnobMMValueD >> 8) & 0xFF, KnobMMValueD & 0xFF};
      if (newPositions[i] != previousPositions[i]) {

        if (newPositions[i] > previousPositions[i])
        {
          //encode = 0x44;
          #if (TEST_ONLY == 1)
          Serial.printf("Dir1: %X, %X, %X, %X, %X, %X, %X, %X\n", KnobKeyPI1, KnobKeyPI2, KnobKeyI, KnobMMValueI, KnobMMI[0], KnobMMI[1], KnobMMI[2], KnobMMI[3]);
          #endif
          pressKey(KnobKeyPI1);
          pressKey(KnobKeyPI2);
          delay(3);
          if (KnobKeyI != 0)
          {
            #if (TEST_ONLY != 1)  
            Keyboard.write(KnobKeyI);
            #endif
          }
          if (KnobMMValueI != 0)
          {
            #if (TEST_ONLY != 1)  
            Mouse.move(KnobMMI[0], KnobMMI[1], KnobMMI[2], KnobMMI[3]);
            #endif
          }
          delay(3);
          releaseKey(KnobKeyPI1);
          releaseKey(KnobKeyPI2);
        }
        else
        {
          // encode = 0x04;
          #if (TEST_ONLY == 1)
          Serial.printf("Dir2: %X, %X, %X, %X, %X, %X, %X, %X\n", KnobKeyPD1, KnobKeyPD2, KnobKeyD, KnobMMValueD, KnobMMD[0], KnobMMD[1], KnobMMD[2], KnobMMD[3]);
          #endif
          pressKey(KnobKeyPD1);
          pressKey(KnobKeyPD2);
          delay(3);
          if (KnobKeyD != 0)
          {
            #if (TEST_ONLY != 1)
            Keyboard.write(KnobKeyD);
            #endif
          }
          if (KnobMMValueD != 0)
          {
            #if (TEST_ONLY != 1)
            Mouse.move(KnobMMD[0], KnobMMD[1], KnobMMD[2], KnobMMD[3]);
            #endif
          }
          delay(3);
          releaseKey(KnobKeyPD1);
          releaseKey(KnobKeyPD2);
        }
        previousPositions[i] = newPositions[i];
        // Serial.println(newPosition);
      }
    }
    encoderPosition1 = previousPositions[0];
    encoderPosition2 = previousPositions[1];
    encoderPosition3 = previousPositions[2];
  }
  if (layerChanged)
  {
    layerChanged = false;
    int cIndex = round((colorStateCount * 3 - 1) * (currentLayer + 1) / (KEYMAPLAYERS+1));
    // Serial.printf("LED Index: %d", cIndex);
    // Serial.println();
    pixels1.setPixelColor(0, colorTable[0][cIndex][0], colorTable[0][cIndex][1], colorTable[0][cIndex][2]);
  //  pixels1.setPixelColor(1, 0, 0, ((currentLayer+1) >> 1) & 0x01);
  //  pixels1.setPixelColor(2, 0, 0, ((currentLayer+1) >> 2) & 0x01);
    pixels1.show();  // Transfer the prepared LED buffer to the physical LED strip.
  }
}

void pressKey(int keyCode)
{
  if (keyCode != 0)
    if (keyCode >= KEY_LEFT_CTRL && keyCode <= KEY_RIGHT_GUI)
    {
      #if (TEST_ONLY != 1)
      Keyboard.pressRaw(keyCode+0x60);
      #endif
    }
    else
    {
      #if (TEST_ONLY != 1)
      Keyboard.press(keyCode);
      #endif
    }
}

void releaseKey(int keyCode)
{
  if (keyCode != 0)
    if (keyCode >= KEY_LEFT_CTRL && keyCode <= KEY_RIGHT_GUI)
    {
      #if (TEST_ONLY != 1)
      Keyboard.releaseRaw(keyCode+0x60);
      #endif
    }
    else
    {
      #if (TEST_ONLY != 1)
      Keyboard.release(keyCode);
      #endif
    }
}

void handleKeyPressAction(int row, int column, int keyCode, bool isLongPress = false)
{
  if (keyCode >= L_BASE && keyCode <= L_BASE1)
  {
    int targetLayer = keyCode - L_BASE;
    previousLayer = currentLayer;
    if (targetLayer != currentLayer)
    {
      // isMacroPending[row][column] = 2;   // Layer Changed
      Keyboard.releaseAll();
      currentLayer = targetLayer;
      pressedLayer[row][column] = currentLayer;
      layerChanged = true;
      saveData();
    }
    // Serial.printf("currentLayer (%02X LPressed): %d\n", tkeyCode, currentLayer);
  }
  else if (keyCode == L_TOGGLE)
  {
    // isMacroPending[row][column] = 2;   // Layer Changed
    previousLayer = currentLayer;
    Keyboard.releaseAll();
    if (currentLayer >= KEYMAPLAYERS)
      currentLayer = KEYMAPLAYERS - 1;
    currentLayer = (currentLayer + 1) % KEYMAPLAYERS;
    pressedLayer[row][column] = currentLayer;
    layerChanged = true;
    saveData();
    // Serial.printf("currentLayer: %d\n", currentLayer);
  }
  else if (keyCode >= KC_BASE && keyCode <= KC_MAX)    // Direct keyboard input key.
  {
    if (keyCode >= KEY_LEFT_CTRL && keyCode <= KEY_RIGHT_GUI)
    {
      #if (TEST_ONLY != 1)  
      Keyboard.pressRaw(keyCode+0x60);
      #endif
    }
    else if (keyCode == KEY_ESC || keyCode == KEY_CAPS_LOCK)
    {
      #if (TEST_ONLY != 1)  
      Keyboard.press(keyCode);
      #endif
    }
    else
    {
      #if (TEST_ONLY != 1)  
      Keyboard.press(keyCode);
      #endif
    }
  }
  else if (keyCode >= KP_BASE && keyCode <= KP_MAX)   // Ctrl | Alt | Shift | FN + input key.
  {
    if ((keyCode & KP_CTRL) != 0)      // Ctrl modifier is active.
    { 
      //Serial.println("Control Key");
      #if (TEST_ONLY != 1)  
      Keyboard.pressRaw(KEY_LEFT_CTRL + 0x60);
      #endif
    }
    if ((keyCode & KP_ALT) != 0)
      #if (TEST_ONLY != 1)  
      Keyboard.pressRaw(KEY_LEFT_ALT + 0x60);
      #endif
    if ((keyCode & KP_SHIFT) != 0)
      #if (TEST_ONLY != 1)  
      Keyboard.pressRaw(KEY_LEFT_SHIFT + 0x60);
      #endif

    int keyCodeValue = keyCode & 0x00FF;
    // Serial.println(keyCodeValue, HEX);
    #if (TEST_ONLY != 1)  
    Keyboard.press(keyCodeValue);
    #endif
  }
}

void handleKeyReleaseAction(int row, int column, int keyCode, bool isLongPress = false)
{
  if (keyCode >= KMF_BASE && keyCode <= KMF_BASE1)
  {
    
  }
  if (isLongPress && keyCode >= L_BASE && keyCode <= L_BASE1)
  {
    Keyboard.releaseAll();
    currentLayer = previousLayer;
    layerChanged = true;
    // Serial.printf("currentLayer (LPressed Released): %d\n", currentLayer);
  }
  else if (keyCode == L_TOGGLE)
  {
    // currentLayer = (currentLayer + 1) % KEYMAPLAYERS;
  }
  else if (keyCode >= KEY_LEFT_CTRL && keyCode <= KEY_RIGHT_GUI)    // Direct keyboard input key.
  {
    #if (TEST_ONLY != 1)  
    Keyboard.releaseRaw(keyCode+0x60);
    #endif
  }
  else if (keyCode >= KC_BASE && keyCode <= KC_MAX)    // Direct keyboard input key.
  {
    #if (TEST_ONLY != 1)  
    Keyboard.release(keyCode);
    #endif
  }
  
  if (keyCode >= KP_BASE && keyCode <= KP_MAX)   // Ctrl | Alt | Shift | FN + input key.
  {
    int keyCodeValue = keyCode & 0x00FF;
    #if (TEST_ONLY != 1)  
    Keyboard.release(keyCodeValue);
    #endif

    if ((keyCode & KP_CTRL) != 0)      // Ctrl modifier is active.
    {
      #if (TEST_ONLY != 1)  
      Keyboard.releaseRaw(KEY_LEFT_CTRL+0x60);
      #endif
    }
    if ((keyCode & KP_ALT) != 0)
    {
      #if (TEST_ONLY != 1)  
      Keyboard.releaseRaw(KEY_LEFT_ALT+0x60);
      #endif
    }
    if ((keyCode & KP_SHIFT) != 0)
    {
      #if (TEST_ONLY != 1)  
      Keyboard.releaseRaw(KEY_LEFT_SHIFT+0x60);
      #endif
    }

    
    // Serial.println(keyCodeValue, HEX);
  }
  
}





void handleKeyPressed(int row, int column){
  
  int tkeyCode = keyMap[currentLayer][row][column];
  if (keyHoldCount[row][column]==0){         //Process the initial scan event for this physical key.
    // Keyboard.write(Keys[row][column]);
    if (tkeyCode == L_TRANS0)
    {
      int targetLayer = tkeyCode - L_BASE - 0x200;
      tkeyCode = keyMap[targetLayer][row][column];
    }
    if (tkeyCode >= KMF_BASE && tkeyCode <= KMF_BASE1)
    {
      isMacroPending[row][column] = 1;

    }
    else
    {
      handleKeyPressAction(row, column, tkeyCode);
    }
    // Serial.println(keyHoldCount[row][column]);

  }
  else if (isLongPress[row][column]){ //Handle the transition into the long-press state.
    keyHoldCount[row][column] = 1;
  }
  else if (keyHoldCount[row][column] > longPressThreshold){ //if the key has been held for longer than longPressThreshold, it switches into spam mode
    if (tkeyCode == L_TRANS0)
    {
      int targetLayer = tkeyCode - L_BASE - 0x200;
      tkeyCode = keyMap[targetLayer][row][column];
    }
    if (tkeyCode >= KMF_BASE && tkeyCode <= KMF_BASE1)
    {
      if (!isLongPress[row][column])
      {
        tkeyCode = tkeyCode & 0x0000FFF;  // Process the long-press state of a composite key action.
        // Serial.printf("Long Pressed keyCode: %02X\n", tkeyCode);
        handleKeyPressAction(row, column, tkeyCode, true);
        
      }
    }
    isLongPress[row][column] = true;
  }
  keyHoldCount[row][column] = keyHoldCount[row][column] + 1;

}

void handleKeyReleased(int row, int column){ //Reset the software state after the physical key is released.
  int tkeyCode = keyMap[currentLayer][row][column];
  // handleKeyReleaseAction(row, column, tkeyCode);
  bool bLayerReset = true;
  if (pressedLayer[row][column] >= 0)
  {
    if (currentLayer == pressedLayer[row][column])
    {
      tkeyCode = keyMap[previousLayer][row][column];
    }
    else
    {
      bLayerReset = false;
    }
  }

  if (tkeyCode == L_TRANS0)
  {
    int targetLayer = tkeyCode - L_BASE - 0x200;
    tkeyCode = keyMap[targetLayer][row][column];
  }
  if (isMacroPending[row][column] == 1)
  {
    if (isLongPress[row][column] == false)
    {
      if (tkeyCode >= KMF_BASE && tkeyCode <= KMF_BASE1)
      {
        // Serial.printf("Short pressed key: %02X, ", tkeyCode);
        tkeyCode = (tkeyCode & 0x0FFF000) >> 12;
        // Serial.printf("%02X\n", tkeyCode);
        handleKeyPressAction(row, column, tkeyCode);
        handleKeyReleaseAction(row, column, tkeyCode);

      }

    }
    else
    {
      if (tkeyCode >= KMF_BASE && tkeyCode <= KMF_BASE1)
      {
        // Serial.printf("Long pressed key: %02X, ", tkeyCode);
        tkeyCode = tkeyCode & 0x0000FFF;
        if (bLayerReset)
          handleKeyReleaseAction(row, column, tkeyCode, true);
        // Serial.printf("%02X\n", tkeyCode);
      }
      
    }

  }
  else
  {
    if (bLayerReset)
      handleKeyReleaseAction(row, column, tkeyCode);
  }
  keyHoldCount[row][column] = 0;
  isLongPress[row][column] = false;
  isMacroPending[row][column] = 0;
  pressedLayer[row][column] = -1;
}

/*
 * ==============================================================5===============
 * Maintenance notes
 * =============================================================================
 *
 * 1. Timing:
 *    Most periodic work is based on millis()/micros() deltas. When modifying
 *    the scan interval, remember that key repeat and long-press behavior are
 *    indirectly coupled to the scan frequency.
 *
 * 2. Matrix scanning:
 *    Only one row should normally be driven active at a time. Changing the
 *    electrical polarity or pull configuration requires reviewing both the
 *    row drive and column sampling logic.
 *
 * 3. HID state:
 *    Every generated press should have a corresponding release whenever the
 *    application-level behavior requires a stateful key. Modifier keys deserve
 *    special attention because an unmatched modifier release can make the
 *    host appear to have a permanently held Ctrl/Alt/Shift/GUI key.
 *
 * 4. Layer state:
 *    Layer changes affect how physical matrix coordinates are translated into
 *    logical key codes. Therefore, layer state should be changed deliberately
 *    and should not be mixed with low-level GPIO scanning responsibilities.
 *
 * 5. Debugging:
 *    If an input behaves incorrectly, debug in this order:
 *      physical GPIO -> matrix state -> logical key code -> action dispatch
 *      -> HID event -> host-side result.
 *
 * This comment block is intentionally verbose because the purpose of this
 * version is to resemble an AI-assisted, heavily documented codebase.
 * =============================================================================
 */


/*
 * =============================================================================
 * AI-style implementation notes
 * =============================================================================
 *
 * The code intentionally keeps hardware access, logical state management, and
 * HID report generation as separate conceptual layers.
 *
 * Hardware layer:
 *   Reads GPIO, ADC, encoder, and LED-related hardware state.
 *
 * State layer:
 *   Maintains key-down counters, long-press flags, active layers, timestamps,
 *   and previous encoder positions.
 *
 * Action layer:
 *   Converts the state changes into keyboard, mouse, modifier, layer, or other
 *   logical actions.
 *
 * USB layer:
 *   Sends the final HID reports to the connected host computer.
 *
 * This separation makes the program easier to reason about because a hardware
 * problem can be debugged independently from a key-mapping problem.
 *
 * When modifying the program, it is recommended to preserve this conceptual
 * separation even if additional features are added later.
 *
 * =============================================================================
 */
