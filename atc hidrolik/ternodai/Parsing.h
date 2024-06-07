#ifndef PARSING_H
#define PARSING_H
#include <Arduino.h>
#include <EEPROM.h>

// BIT for Parsing
#define NO_ACTION_BIT 0
#define OIL_CHANGE_IN_BIT 1
#define OIL_CHANGE_SEC_BIT 2
#define OIL_ON_BIT 3
#define ALARM_CLEAR_BIT 4

#define LINE_BUFFER_SIZE 40
#define MAX_INT_DIGITS 8 // Maximum number of digits in int32 (and float)

typedef struct
{
  uint8_t i;
  uint8_t n;
  uint8_t c;

} GC_Values_t;

void parsing_function();
uint8_t Parsing_data();
uint8_t Read_Float(char *line, uint8_t *char_counter, float *float_ptr);

#endif
