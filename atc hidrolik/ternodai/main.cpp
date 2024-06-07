#include <Arduino.h>
#include <orientation.h>
#include <parsing.h>
// emergency stop
bool emergency = false;
byte keadaanEmergencyTerakhir = LOW;
unsigned long lastTimeButtonStateChanged = 0;

void setup()
{
  // Serial.begin(9600);
  pinMode(PA3, INPUT_PULLUP);
  // INISIALISASI AWAL ATC (POSISI LOCK, DAN OFF CW & CCW NYA)
  pinOut[0].ON();
  pinOut[1].OFF();
}

void loop()
{
  if (millis() - lastTimeButtonStateChanged > 500)
  {
    byte keadaanEmergency = digitalRead(PA3);
    if (keadaanEmergency != keadaanEmergencyTerakhir)
    {
      lastTimeButtonStateChanged = millis();
      keadaanEmergencyTerakhir = keadaanEmergency;
      // artinya tertrigger (karena emergency default low)
      keadaanEmergency == HIGH ? emergency = true : emergency = false;
    }
  }

  if (emergency)
  {
    pinOut[0].ON();
    pinOut[1].OFF();
  }
  else
  {
    parsing_function();
    loop_orient();
  }
}
