#include <Arduino.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include "ecp.h"

#define RS_MODE_PIN 7
#define RELAY_PIN 4
#define LCD_WIDTH 16
#define LCD_HEIGHT 2
#define KEYPAD_ROWS 5
#define KEYPAD_COLS 1

//function declerations
void controlScreen(char key);
void stopFillWhenScreen(char key);
void refillWhenScreen(char key);
void sensorHeightScreen(char key);
void waterDepthScreen(char key);

void updateLCD(String line1, String line2);
void setPumpOnOrOff(boolean isOn);

int getStopFillWhen();
int getRefillWhen();
int getSensorHeight();
int getWaterDepth();

void setStopFillWhen(int value);
void setRefillWhen(int value);
void setSensorHeight(int value);
void setWaterDepth(int value);

int screenIndex = 0;
int waterLevel = 0;
bool pumpOn = false;
String controlStates[] = {"OFF", "AUTO", "FILL & AUTO", "FILL & OFF"};
int controlStateIndex = 0; // 0 = OFF, 1 = AUTO, 2 = FILL & AUTO, 3 = FILL & OFF 
int local_controlStateIndex = controlStateIndex;
// int stopFillWhen = 80;
// int refillWhen = 20;
// int sensorHeight = 20;
// int waterDepth = 200;
String padding = "";

// //Keypad/////////////////////////////////////////////////////////////////
char hexaKeys[KEYPAD_ROWS][KEYPAD_COLS] = {{'N'}, {'B'}, {'I'}, {'D'}, {'S'}};
byte rowPins[KEYPAD_ROWS] = {A0, A1, A2, A3, 6};
byte colPins[KEYPAD_COLS] = {5}; 
Keypad kpd = Keypad(makeKeymap(hexaKeys), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);
// ////////////////////////////////////////////////////////////////////////

typedef void (*Screens)(char key);
Screens screens[] = {
  controlScreen, stopFillWhenScreen, refillWhenScreen, sensorHeightScreen, waterDepthScreen
};

LiquidCrystal_I2C lcd(0x27, LCD_WIDTH, LCD_HEIGHT);

void setup() {

  // int stopFillWhen = 80;
  // int refillWhen = 20;
  // int sensorHeight = 20;
  // int waterDepth = 200;

  // setStopFillWhen(80);
  // setRefillWhen(20);
  // setSensorHeight(20);
  // setWaterDepth(200);

  lcd.init();
  lcd.clear();
  lcd.backlight();

  for(int j = 0; j < LCD_WIDTH; j++){
    padding += ' ';
  }
  
  pinMode(RS_MODE_PIN, OUTPUT);
  digitalWrite(RS_MODE_PIN, LOW); // if low, is receiver

  pinMode(RELAY_PIN, OUTPUT);
  setPumpOnOrOff(false);

  Serial.begin(9600);

  screens[screenIndex]('z');
}

void loop() {

  // 13 Carriage return
  // 10 NL feed

  if(controlStateIndex == 1)// 0 = OFF, 1 = AUTO, 2 = FILL & AUTO, 3 = FILL & OFF 
  {
    if(waterLevel >= getStopFillWhen() && pumpOn == true)
    {
      setPumpOnOrOff(false);
    }
    else if(waterLevel <= getRefillWhen() && pumpOn == false)
    {
      setPumpOnOrOff(true);
    }
  }
  else if(controlStateIndex == 2) // FILL & AUTO
  {
    if(waterLevel >= getStopFillWhen() && pumpOn == true)
    {
      setPumpOnOrOff(false); //pump off when done
      controlStateIndex = 1; //SYS AUTO when done
      local_controlStateIndex = 1;
    }
  }
  else if(controlStateIndex == 3) // FILL & OFF
  {
    if(waterLevel >= getStopFillWhen() && pumpOn == true)
    {
      setPumpOnOrOff(false); //pump off when done
      controlStateIndex = 0; //SYS OFF when done
      local_controlStateIndex = 0;
    }
  }


  if(Serial.available() > 0)
  {
    char incomingByte =  Serial.read(); // for incoming serial data
      
    String message = receiveMessage(incomingByte);
    if(message != "")
    {
      String sensorReadingStr = getValueOfKey("SR", message, "num");
      if(sensorReadingStr != "")
      {
        // int reading = sensorReadingStr.toInt();
        int senReading = sensorReadingStr.toInt();
        float senReading_heightAdjusted = getWaterDepth() - (senReading - getSensorHeight() );
        int calculatedWaterLevel = (senReading_heightAdjusted / getWaterDepth()) * 100;

        if(calculatedWaterLevel != waterLevel)
        {
          if(calculatedWaterLevel > waterLevel)
          {
            waterLevel++;
          }
          else
          {
             waterLevel--;
          }
          if(screenIndex == 0)
          {
            screens[screenIndex]('z');
          }
        }
      }
    }
  }

  char key = kpd.getKey();
  if(key)
  {
    Serial.println(key);
    if(key == 'N')
    {
      screenIndex++;
      if(screenIndex > 4)
      {
        screenIndex = 0;
      }
    }
    else if(key == 'B')
    {
      screenIndex--;
      if(screenIndex < 0)
      {
        screenIndex = 4;
      }
    }

    screens[screenIndex](key);
  }
}

// put function definitions here:
void controlScreen(char key){

  if(key == 'N' || key == 'B')
  {
    local_controlStateIndex = controlStateIndex;
  }
  else if(key == 'I')
  {
    local_controlStateIndex++;
    if(local_controlStateIndex > 3)
    {
      local_controlStateIndex = 0;
    }
  }
  else if(key == 'D')
  {
    local_controlStateIndex--;
    if(local_controlStateIndex < 0)
    {
      local_controlStateIndex = 3;
    }
  }
  else if(key == 'S')
  {
    controlStateIndex = local_controlStateIndex;
    if(controlStateIndex == 0) //OFF
    {
      setPumpOnOrOff(false);
    } 
    else if(controlStateIndex == 1 && waterLevel <= getRefillWhen()) //AUTO
    {
      setPumpOnOrOff(true);
    } 
    else if((controlStateIndex == 2 || controlStateIndex == 3) && waterLevel < getStopFillWhen())
    {
      setPumpOnOrOff(true);
    }
  }


  String line2 = "";
  if(local_controlStateIndex != controlStateIndex)
  {
    line2 = "Set:" + controlStates[local_controlStateIndex];
  } 
  else
  {
    line2 = controlStates[local_controlStateIndex];
  }

  const String pumpStatus = pumpOn ? "ON " : "OFF";
  updateLCD(
    "Ctrl P:" + pumpStatus + " " + waterLevel + "%", 
    line2
  );

}

void stopFillWhenScreen(char key)
{
  static int local_stopFillWhen = 0;

  if(key == 'N' || key == 'B')
  {
    local_stopFillWhen = getStopFillWhen();
  }
  else if(key == 'I' && local_stopFillWhen + 5 <= 100 )
  {
    local_stopFillWhen += 5;
  }
  else if(key == 'D' && getRefillWhen() < local_stopFillWhen - 5)
  {
    local_stopFillWhen -= 5;
  }
  else if(key == 'S')
  {
    setStopFillWhen(local_stopFillWhen);
  }

  String line2 = "";
  if(local_stopFillWhen != getStopFillWhen())
  {
    line2 = "Set: " + String(local_stopFillWhen) + "%";
  } 
  else
  {
    line2 = String(local_stopFillWhen) + "%";
  }

  updateLCD("Stop fill when:", line2);
}

void refillWhenScreen(char key)
{
  static int local_refillWhen = 0;

  if(key == 'N' || key == 'B')
  {
    local_refillWhen = getRefillWhen();
  }
  else if(key == 'I' && local_refillWhen + 5 < getStopFillWhen() )
  {
    local_refillWhen += 5;
  }
  else if(key == 'D' && local_refillWhen - 5 >= 0)
  {
    local_refillWhen -= 5;
  }
  else if(key == 'S')
  {
    setRefillWhen(local_refillWhen);
  }


  String line2 = "";
  if(local_refillWhen != getRefillWhen())
  {
    line2 = "Set: " + String(local_refillWhen) + "%";
  } 
  else
  {
    line2 = String(local_refillWhen) + "%";
  }

  updateLCD("Refill when:", line2);
}

void sensorHeightScreen(char key)
{
  static int local_sensorHeight = 0;

  if(key == 'N' || key == 'B')
  {
    local_sensorHeight = getSensorHeight();
  }
  else if(key == 'I' && (local_sensorHeight + 5 + getWaterDepth() <= 600) )
  {
    local_sensorHeight += 5;
  }
  else if(key == 'D' && local_sensorHeight - 5 >= 20)
  {
    local_sensorHeight -= 5;
  }
  else if(key == 'S')
  {
    setSensorHeight(local_sensorHeight);
  }

  String line2 = "";
  if(local_sensorHeight != getSensorHeight())
  {
    line2 = "Set: " + String(local_sensorHeight) + "cm";
  } 
  else
  {
    line2 = String(local_sensorHeight) + "cm";
  }

  updateLCD("Sensor H abv W", line2);
}

void waterDepthScreen(char key)
{
  static int local_waterDepth = 0;

  if(key == 'N' || key == 'B')
  {
    local_waterDepth = getWaterDepth();
  }
  else if(key == 'I' && local_waterDepth + 5 + getSensorHeight() <= 600)
  {
    local_waterDepth += 5;
  }
  else if(key == 'D' && getWaterDepth() - 5 >= 10)
  {
    local_waterDepth -= 5;
  }
  else if(key == 'S')
  {
    setWaterDepth(local_waterDepth);
  }

  String line2 = "";
  if(local_waterDepth != getWaterDepth())
  {
    line2 = "Set: " + String(local_waterDepth) + "cm";
  } 
  else
  {
    line2 = String(local_waterDepth) + "cm";
  }

  updateLCD("Water Depth", line2);
}

void updateLCD(String line1, String line2)
{
  lcd.setCursor(0, 0);
  lcd.print(line1 + padding);   
  lcd.setCursor(0, 1);
  lcd.print(line2 + padding);      
}

void setPumpOnOrOff(boolean isOn){
  if(isOn)
  {
    digitalWrite(RELAY_PIN, HIGH);
    pumpOn = isOn;
  }
  else
  {
    digitalWrite(RELAY_PIN, LOW);
    pumpOn = isOn;
  }
}

//Get values from EERPOM

int getStopFillWhen()
{
  int stopFillWhen = EEPROM.read(0);
  return stopFillWhen;
}

int getRefillWhen(){
  int refillWhen = EEPROM.read(1);
  return refillWhen;
}

int getSensorHeight()
{
  int sensorHeight = EEPROM.read(2);
  return sensorHeight * 5;
}

int getWaterDepth()
{
  int waterDepth = EEPROM.read(3);
  return waterDepth * 5;
}

//Set values in EEPROM

void setStopFillWhen(int value)
{
  EEPROM.write(0, value);
}

void setRefillWhen(int value)
{
  EEPROM.write(1, value);
}

void setSensorHeight(int value)
{
  EEPROM.write(2, value / 5);
}

void setWaterDepth(int value)
{
  EEPROM.write(3, value / 5);
}
