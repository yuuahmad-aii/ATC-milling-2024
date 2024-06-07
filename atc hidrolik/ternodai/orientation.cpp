#include <Arduino.h>
#include <orientation.h>
#include <cpu_map.h>

#ifdef SPINDLE_ORIENT
#pragma message "ORIENTATION ACTIVE"
int data[] = {0, 0, 0, 0, 0, 0, 0, 0};

ATC::ATC(int pinNumber, int mode) // nomor pin, mode (1 output, 0 input_pullup)
{
    pin = pinNumber;
    if (mode == 1)
    {
        pinMode(pin, OUTPUT);
        OFF();
    }
    else
    {
        pinMode(pin, INPUT_PULLUP);
    }
}

void ATC::setInputBounce(int value)
{
    value = bounce;
}
void ATC::OFF()
{
    digitalWriteFast(digitalPinToPinName(pin), 0);
}
void ATC::ON()
{
    digitalWriteFast(digitalPinToPinName(pin), 1);
}

bool ATC::state()
{
    if (millis() - timestamp > bounce)
        timestamp = 0;
    if (!digitalReadFast(digitalPinToPinName(pin)))
    {
        if (millis() - timestamp > 50)
        {
            status = true;
        }
    }
    else
    {
        timestamp = millis();
        status = false;
    }
    return status;
}

// Deklarasi array objek ATC INPUT (proximity)
ATC Triggr[] = {
    ATC(INPUT0, 0), // b9
    ATC(INPUT1, 0), // b8
    ATC(INPUT2, 0), // a8
    ATC(INPUT3, 0), // b10
    ATC(INPUT4, 0), // b5
    ATC(INPUT5, 0), // b4
    // ATC(INPUT6, 0), // b3 //ini untuk emergency stop (ada di main)
};

// Deklarasi array objek ATC OUTPUT
ATC pinOut[] = {
    ATC(OUTPUT0, 1),
    ATC(OUTPUT1, 1),
    ATC(OUTPUT2, 1),
    ATC(OUTPUT3, 1),
};

void loop_orient()
{
    // baca status pin input, kemudian kirim data ke PC
    Serial.print("P");
    for (int i = 0; i < 6; i++)
    {
        data[i] = Triggr[i].state();
        Serial.print(data[i]);
    }
    Serial.println("");
}
#endif