#ifndef OIL_H
#define OIL_H

#include <Arduino.h>
// #include <EEPROM.h>
// OIL

#define OIL_STATE_DISABLE 0 // Must be zero.
#define OIL_STATE_ENABLE 1

#define OIL_LEVEL_OK 0
#define OIL_LEVEL_ALARM 1

extern uint8_t ReadC;
extern uint8_t ReadOn;

void tes_oilpump_buzzer();
void Oil_init();
void Oil_F();
void Oil_Run();
void Oil_PumpF_Pause();
void Oil_PumpF_Resume();
uint8_t Oil_Pump_GetState();
uint8_t Get_Oil_level();
uint8_t Oil_Int_Sync(uint8_t interval);
uint8_t Oil_On_Sync(uint8_t On);
void Oil_On();

#endif /* OIL_H */
