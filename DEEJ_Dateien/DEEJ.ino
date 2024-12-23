#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include <CircularBuffer.h>

#define PIXEL_COUNT 10
#define PIXEL_PIN 9
#define BUTTON_COUNT 5

#define gameRounds 5

#define buttonColorCommand 1
#define saveCommand 2
#define loadCommand 3
#define disableSendCommand 4
#define colorsAdress 1
long sleepTime= 300;// sek
int lastPressedVal, oldLastPressedVal = 9999999999;
long standbytime;
bool standby;
uint8_t Colors[BUTTON_COUNT][3] = {{255, 255, 255}, {255, 120, 2}, {50, 33, 176}, {232, 240, 2}, {0, 255, 0}};
uint8_t ausColor[3] = {255, 0, 0};
uint8_t Weiss[3] = {255, 255, 255};
uint8_t offColor[3] = {0, 0, 0};
uint8_t Magic[3] = {21, 35, 44};
const uint8_t BUTTON_PINS[BUTTON_COUNT] = {2, 3, 4, 5, 6};
Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

const int NUM_SLIDERS = 5;
const int analogInputs[NUM_SLIDERS] = {A0, A1, A2, A3, A4};
int buttonStates[BUTTON_COUNT];
bool buttonPressed[BUTTON_COUNT], buttonSettedBrightness[BUTTON_COUNT];
int analogSliderValues[NUM_SLIDERS];
int oldAnalogSliderValues[NUM_SLIDERS];
int brightness = 255;
long pressedTime[BUTTON_COUNT]; // Separate time tracking for each button
float glowInterval = 0.7, glowAmp = 0.5;
bool disableSend = false;
uint32_t Magic_Wow = 1385260;
long lastSend;
struct Msg
{
  uint8_t Colors[BUTTON_COUNT][2][3];
  uint32_t MAGIC_WOW;
};
CircularBuffer<uint8_t, sizeof(Msg)> buffer;

void setup()
{
  Serial.begin(9600);
  loadColors();
  for (int i = 0; i < BUTTON_COUNT; i++)
  {
    pinMode(BUTTON_PINS[i], INPUT);
    pressedTime[i] = 0; // Initialize press time
  }
  strip.begin();
  brightness = EEPROM.read(0);
  strip.setBrightness(brightness);
  onAnimation();
}

void loop()
{

  updateButtonValues();
  updateSliderValues();
  if (abs(oldLastPressedVal - lastPressedVal) > 45)
  {
    standby = false;
    standbytime = millis();
    oldLastPressedVal = lastPressedVal;
  }
  else if ((millis() - standbytime)/1000. > sleepTime)
  {
    standby = true;
 
  }

  if (millis() - lastSend > 100 && !disableSend)
  {
    sendSliderValues();
    lastSend = millis();
  }

  if (Serial.available())
  {
    buffer.unshift(Serial.read());
    if (buffer[1] == 21 && buffer[2] == 35 && buffer[3] == 44)
    {
      byte Command = buffer[0];
      switch (Command)
      {
      case buttonColorCommand:
        loadColorsFromBuffer();
        break;
      case saveCommand:
        saveColors();
        break;
      case loadCommand:
        loadColors();
        disableSend = true;
        sendButtonColors();
        break;
      case disableSendCommand:
        disableSend = true;
        break;
      }
    }
  }
}

void updateButtonValues()
{
  int pressedCount = 0;

  for (int i = 0; i < BUTTON_COUNT; i++)
  {
    bool isPressed = digitalRead(BUTTON_PINS[i]);
    if (isPressed)
      lastPressedVal += 50;
    if (isPressed)
    {
      if (!buttonPressed[i])
      {
        buttonPressed[i] = true;
        pressedTime[i] = millis(); // Start timing when button is pressed
      }

      // Check if button is held for 300ms or more
      if (millis() - pressedTime[i] >= 300)
      {
        buttonSettedBrightness[i] = true;
        if (i == 0)
        {
          brightness = max(0, brightness - 20); // Decrease brightness
          strip.setBrightness(brightness);
          pressedTime[i] = millis(); // Reset timer for repeated adjustments
        }
        else if (i == 4)
        {
          brightness = min(255, brightness + 20); // Increase brightness
          strip.setBrightness(brightness);
          pressedTime[i] = millis(); // Reset timer for repeated adjustments
        }
        else if (i == 1)
        {
          brightness = min(255, brightness - 2); // Increase brightness
          strip.setBrightness(brightness);
          pressedTime[i] = millis(); // Reset timer for repeated adjustments
        }
        else if (i == 3)
        {
          brightness = min(255, brightness + 2); // Increase brightness
          strip.setBrightness(brightness);
          pressedTime[i] = millis(); // Reset timer for repeated adjustments
        }
        else if (i == 2)
        {
          game();
        }
        EEPROM.update(0, brightness);
      }
    }
    else if (!isPressed && buttonPressed[i] && !buttonSettedBrightness[i])
    {
      buttonStates[i] = !buttonStates[i];
      buttonPressed[i] = false;
    }
    else
    {
      buttonPressed[i] = false; // Reset state if button is released
      buttonSettedBrightness[i] = false;
    }

    if (buttonStates[i])
    {
      lightButton(i, ausColor, true); // Activate flicker
    }
    else
    {
      lightButton(i, Colors[i], false); // Static colors
    }
    if (standby)strip.setBrightness(1);
    else strip.setBrightness(brightness);
  }

  strip.show();
}
void sendButtonColors()
{

  Serial.write(3);
  for (int i = 0; i < 3; i++)
  {
    Serial.write(Magic[i]);
  }

  for (int i = 0; i < 5; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      Serial.write(Colors[i][j]);
    }
  }

  // 1x3-Array (Ende-Identifikation) senden

  // Einzelwert (Command Identifier) senden
}
void updateSliderValues()
{
  for (int i = 0; i < NUM_SLIDERS; i++)
  {
    int multi = 1;
    if (buttonStates[i])
      multi = 0;
    int tmp = analogRead(analogInputs[i]);
    int tmp1 = 0;
    if (tmp > 900)
    {
      tmp1 = map(tmp, 900, 1023, 1023 * 0.75, 1023);
    }
    else if (tmp < 100)
    {
      tmp1 = map(tmp, 0, 100, 0, 1023 * 0.25);
    }
    else
      tmp1 = map(tmp, 100, 900, 1023 * 0.26, 1023 * 0.76);
    oldAnalogSliderValues[i] = analogSliderValues[i];
    analogSliderValues[i] = (1023 - tmp1) * multi;
    lastPressedVal += oldAnalogSliderValues[i] - analogSliderValues[i];
  }
}
void saveColors()
{
  for (uint8_t i = 0; i < 5; i++)
  {
    for (uint8_t j = 0; j < 3; j++)
    {
      EEPROM.update(colorsAdress + (i * 3 + j), Colors[i][j]);
    }
  }
}
void loadColors()
{
  for (uint8_t i = 0; i < 5; i++)
  {
    for (uint8_t j = 0; j < 3; j++)
    {
      Colors[i][j] = EEPROM.read(colorsAdress + (i * 3 + j));
    }
  }
}
void lightButton(int index, uint8_t color[3], bool flicker)
{
  int modColor[3];

  if (flicker)
  {
    float sinValue = sin(millis() * 2 * PI / (1000.0 * glowInterval)) * glowAmp / 2 - glowAmp;
    for (int i = 0; i < 3; i++)
    {
      modColor[i] = color[i] + (int)(color[i] * sinValue);
      modColor[i] = color[i] + (int)(color[i] * sinValue);
    }
  }
  else
  {
    for (int i = 0; i < 3; i++)
    {
      modColor[i] = color[i];
      modColor[i] = color[i];
    }
  }

  strip.setPixelColor(BUTTON_COUNT - index - 1, strip.Color(modColor[0], modColor[1], modColor[2]));
  strip.setPixelColor(BUTTON_COUNT + index, strip.Color(modColor[0], modColor[1], modColor[2]));
}
void sendSliderValues()
{
  String builtString = String("");

  for (int i = 0; i < NUM_SLIDERS; i++)
  {
    builtString += String((int)analogSliderValues[i]);

    if (i < NUM_SLIDERS - 1)
    {
      builtString += String("|");
    }
  }

  Serial.println(builtString);
}
void offButton(int i)
{
  lightButton(i, offColor, false);
}
void turnOffButtons()
{

  for (int a = 0; a < BUTTON_COUNT; a++)
  {
    offButton(a);
  }
}
void onAnimation()
{
  turnOffButtons();

  for (int a = BUTTON_COUNT - 1; a >= 0; a--)
  {
    for (int i = 0; i < BUTTON_COUNT; i++)
    {
      lightButton(a + i, Colors[i], false);
    }
    strip.show();
    delay(200);
  }
}
void game()
{
  uint16_t Score = 0;
  while (digitalRead(BUTTON_PINS[2]))
    ;
  delay(1000);
  for (int a = 0; a < 3; a++)
  {
    offButton(a);
    offButton(BUTTON_COUNT - a - 1);
    strip.show();
    delay(1000);
  }
  int colorsIndex = random(BUTTON_COUNT);
  for (int i = 0; i < gameRounds; i++)
  {
    Score += Round(colorsIndex);
  }
  Score /= gameRounds;

  uint16_t Scores[BUTTON_COUNT];
  for (int i = 0; i < BUTTON_COUNT; i++)
  {
    Scores[i] = readUint16FromEEPROM(1 + i * sizeof(uint16_t));
  }

  int Scoreindex = 0;
  int i = 0;
  /*turnOffButtons();
   strip.show();
   delay(1000);
   if (Scoreindex != -1)
   {
     lightButton(Scoreindex, Weiss, false);
     strip.show();
     insertValue(Score);
   }*/
  delay(2000);
}
float Round(int colorsIndex)
{

  int Button = 0;
  Button = random(BUTTON_COUNT);
  int color[2][3] = {{0, 0, 0}, {0, 0, 0}};
  long time;

  lightButton(Button, Colors[colorsIndex], false);
  delay(300);
  time = millis();
  strip.show();
  while (!digitalRead(BUTTON_PINS[Button]))
    ;
  offButton(Button);
  strip.show();
  return (millis() - time);
}
void saveUint16ToEEPROM(int address, uint16_t value)
{
  // Das High-Byte und das Low-Byte der uint16_t speichern
  EEPROM.write(address, value >> 8);       // High-Byte
  EEPROM.write(address + 1, value & 0xFF); // Low-Byte
}

// Funktion zum Lesen einer uint16_t aus dem EEPROM
uint16_t readUint16FromEEPROM(int address)
{
  // Das High-Byte und das Low-Byte lesen und wieder zusammensetzen
  uint8_t highByte = EEPROM.read(address);
  uint8_t lowByte = EEPROM.read(address + 1);
  return (highByte << 8) | lowByte;
}
// Funktion zum Einfügen eines Wertes in die sortierte Liste
int insertValue(uint16_t newValue)
{
  uint16_t values[BUTTON_COUNT];
  // Temporäres Array für die neue Liste
  for (int i = 0; i < BUTTON_COUNT; i++)
  {
    values[i] = readUint16FromEEPROM(1 + i * sizeof(uint16_t));
  }

  uint16_t temp[BUTTON_COUNT + 1];

  // Kopiere aktuelle Werte in das temporäre Array
  for (int i = 0; i < BUTTON_COUNT; i++)
  {
    temp[i] = values[i];
  }

  // Füge den neuen Wert hinzu
  temp[BUTTON_COUNT] = newValue;

  // Sortiere das Array (groß -> klein)
  for (int i = 0; i < BUTTON_COUNT + 1; i++)
  {
    for (int j = i + 1; j < BUTTON_COUNT + 1; j++)
    {
      if (temp[i] > temp[j])
      {
        uint16_t tempValue = temp[i];
        temp[i] = temp[j];
        temp[j] = tempValue;
      }
    }
  }

  // Speichere nur die ersten 5 Werte zurück
  for (int i = 0; i < BUTTON_COUNT; i++)
  {
    saveUint16ToEEPROM(1 + i * sizeof(uint16_t), temp[i]);
  }
}
void loadColorsFromBuffer()
{
  int index = 18; // Linearer Index für den Circular Buffer

  for (uint8_t i = 0; i < 5; i++)
  {
    for (uint8_t j = 0; j < 3; j++)
    {
      if (index < buffer.size())
      {
        Colors[i][j] = buffer[index];
        index--;
      }
    }
  }
}
