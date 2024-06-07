#include "Parsing.h"
#include "orientation.h"

// Parsing
static char line[LINE_BUFFER_SIZE];
String dt[8];
int i;
bool parsing = false;
uint8_t action = 0;
static size_t buffer_pos = 0;
GC_Values_t gc_state;

void parsing_function()
{
  if (Serial.available() > 0)
  {
    char c = Serial.read();
    c = toUpperCase(c);
    if (c == '\n')
    {                          // end of message
      line[buffer_pos] = '\0'; // terminate the string
      parsing = true;
      if (parsing)
      {
        Parsing_data();
        action = NO_ACTION_BIT;
        parsing = false;
        buffer_pos = 0; // reset for next message
      }
    }
    else if (buffer_pos < sizeof line - 1)
    {
      line[buffer_pos++] = c; // buffer the character
    }
  }
}

uint8_t Parsing_data()
{
  uint8_t char_counter = 0;
  char letter = 0;
  float value = 0.0;
  uint8_t int_value = 0;
  while (line[char_counter] != 0) // Loop until no more g-code words in line.
  {
    letter = line[char_counter];
    char_counter++;
    int_value = trunc(value);
    switch (letter)
    {
    case 'L': // LOCK ATC
      pinOut[0].ON();
      pinOut[1].OFF();
      // pinOut[2].OFF();
      // pinOut[3].OFF();
      break;
    case 'U':          // UNLOCK ATC
      pinOut[0].OFF(); // PIN LOCK
      pinOut[1].ON();  // PIN UNLOCK
      // pinOut[2].OFF();
      // pinOut[3].OFF();
      break;
    case 'A': // BERPUTAR CW
      // original A
      // pinOut[0].OFF(); // PIN LOCK
      // pinOut[1].ON();  // PIN UNLOCK
      // pinOut[2].ON();  // PIN CW
      // pinOut[3].OFF(); // PIN CCW
      pinOut[0].OFF(); // PIN LOCK
      pinOut[1].ON();  // PIN UNLOCK
      pinOut[2].OFF();
      pinOut[3].ON();
      break;
    case 'B': // BERPUTAR CCW
      // original B
      pinOut[0].OFF(); // PIN LOCK
      pinOut[1].ON();  // PIN UNLOCK
      pinOut[2].OFF();
      pinOut[3].ON();
      // pinOut[0].OFF(); // PIN LOCK
      // pinOut[1].ON();  // PIN UNLOCK
      // pinOut[2].ON();
      // pinOut[3].OFF();
      break;
    case 'C':          // BERHENTI BERPUTAR
      pinOut[0].OFF(); // PIN LOCK
      pinOut[1].ON();  // PIN UNLOCK
      pinOut[2].OFF();
      pinOut[3].OFF();
      // pinOut[2].ON();
      // pinOut[3].ON();
      break;
    case 'N':
      break;
    default:
      // Serial.println(letter);
      if (line[char_counter] == 0)
      {
      }
    }
  } // complete parsing
  return 1;
}