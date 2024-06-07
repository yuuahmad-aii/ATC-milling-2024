#include "Oil.h"
#include <EEPROM.h>

// pinout oil lubricants
#define Oil_Pump PA5
#define On_Oil_Timer PA6
#define Oil_level_sensor PA7
#define Buzzer PB0

unsigned long PrevMillis;
unsigned long counterMillis;
unsigned long on_timeMillis;
unsigned long count_prev_timeMillis;
unsigned long SetParams;
bool resume_time;
bool Oil_Timer;
bool pause_time;
int Second;

void tes_oilpump_buzzer()
{
  digitalWriteFast(digitalPinToPinName(Oil_Pump), HIGH);
  digitalWriteFast(digitalPinToPinName(Buzzer), HIGH);
  delay(1000);

  digitalWriteFast(digitalPinToPinName(Oil_Pump), LOW);
  digitalWriteFast(digitalPinToPinName(Buzzer), LOW);
  delay(1000);
}

void Oil_init()
{
  pinMode(Oil_Pump, OUTPUT);
  pinMode(On_Oil_Timer, INPUT);
  pinMode(Oil_level_sensor, INPUT_PULLUP);
  pinMode(Buzzer, OUTPUT);
  digitalWrite(Buzzer, LOW);

  digitalWrite(Oil_Pump, LOW);

  ReadC = EEPROM.read(0);
  ReadOn = EEPROM.read(1);
  if (ReadC > 99 || ReadC <= 0)
    EEPROM.update(0, 1);
  if (ReadOn > 60 || ReadOn <= 0)
    EEPROM.update(1, 5);
  ReadC = EEPROM.read(0);
  ReadOn = EEPROM.read(1);

  resume_time = 0;
  Oil_Timer = 1;
  pause_time = 1; // hidup selama 2 detik
  SetParams = 1000;
  Second = 1; // waktu delay
}

void Oil_F()
{
  if ((digitalRead(On_Oil_Timer) == LOW))
  { // Spindle Off
    digitalWrite(Oil_Pump, 0);
    Oil_PumpF_Pause();
    Oil_Timer = 0;
  }
  else if ((digitalRead(On_Oil_Timer) == HIGH) && (!Oil_Timer))
  {
    Oil_PumpF_Resume();
    // Serial.println("resume");
  }
  if (Oil_Timer)
  {
    Oil_Run();
    // Serial.println(Oil_Timer);
  }
}

void Oil_Run()
{
  unsigned long Result = (ReadC * Second) * SetParams;
  counterMillis = millis();
  // Serial.println("Run");
  if ((unsigned long)(counterMillis - PrevMillis) >= Result)
  {
    on_timeMillis = millis();
    digitalWrite(Oil_Pump, HIGH);
    // Serial.println("OLI_ON");
    PrevMillis = counterMillis;
  }
  if ((digitalRead(Oil_Pump) == 1) && ((millis() - on_timeMillis) >= (ReadOn * 1000)))
  {
    digitalWrite(Oil_Pump, LOW);
    // Serial.println("OLI_OFF");
  }
}

void Oil_PumpF_Pause()
{
  counterMillis = millis();
  // Serial.println("Pause");
  if (pause_time)
  {
    count_prev_timeMillis += (unsigned long)(counterMillis - PrevMillis); // waktu yang sudah berjalan
    pause_time = 0;
  }
  resume_time = 1;
}

void Oil_PumpF_Resume()
{
  pause_time = 1;
  // Serial.println("Resume");
  if (count_prev_timeMillis == 0)
  {
    Oil_Timer = 1; // enter Oil_ON
    return;
  }
  else
  {
    if (resume_time)
    {
      resume_time = 0;
      PrevMillis = millis();
    }
  }

  unsigned long Result_resume = ((ReadC * Second) * SetParams) - count_prev_timeMillis;
  counterMillis = millis();

  if ((unsigned long)(counterMillis - PrevMillis) >= Result_resume)
  {
    on_timeMillis = millis();
    PrevMillis = counterMillis;
    Result_resume == 0;
    digitalWrite(Oil_Pump, HIGH);
    // Serial.println("OLI_ON");

    count_prev_timeMillis = 0;
    Oil_Timer = 1;
  }
  if ((digitalRead(Oil_Pump) == 1) && ((millis() - on_timeMillis) >= (ReadOn * 1000)))
  {
    digitalWrite(Oil_Pump, LOW);
    // Serial.println("OLI_OFF");
    count_prev_timeMillis = 0;
  }
}

uint8_t Oil_Pump_GetState()
{
  uint8_t oil_state = OIL_STATE_DISABLE;
  if (digitalRead(Oil_Pump))
  {
    oil_state |= OIL_STATE_ENABLE;
    // Serial.print(oil_state);
  }
  return oil_state;
}

uint8_t Get_Oil_level()
{
  uint8_t oil_level = OIL_LEVEL_OK;
  if (digitalRead(Oil_level_sensor))
  {
    oil_level = OIL_LEVEL_ALARM;
  }
  if (oil_level)
  {
    digitalWrite(Buzzer, HIGH);
  }
  else
  {
    digitalWrite(Buzzer, LOW);
  }
  // Serial.println(oil_level);
  return oil_level;
}

uint8_t Oil_Int_Sync(uint8_t interval)
{
  // Serial.println(interval);
  if (interval > 99 || interval == 0)
  {
    return 0;
  }
  else
  {
    EEPROM.update(0, interval);
  }
  ReadC = EEPROM.read(0);
  return 0;
}

uint8_t Oil_On_Sync(uint8_t On)
{
  if (On > 99 || On == 0)
  {
    return 0;
  }
  else
  {
    EEPROM.update(1, On);
  }

  ReadOn = EEPROM.read(1);
  return 0;
}

void Oil_On()
{
  digitalWrite(Oil_Pump, HIGH);
  delay(ReadOn * 1000);
  digitalWrite(Oil_Pump, LOW);
}
