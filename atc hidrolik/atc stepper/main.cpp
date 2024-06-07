#include <Arduino.h>
#include "AccelStepper.h"

// Define stepper motor connections and motor interface type.
// Motor interface type must be set to 1 when using a driver

// #define stepPin PB11
#define stepPin PC13         // PIN CW (step )
#define dirPin PC14          // PIN CCW (dir)
#define motorInterfaceType 1 // artinya menggunakan stepper driver

// Create a new instance of the AccelStepper class:
AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);

bool motorRunning = false;

void setup()
{
    Serial.begin(9600); // Start serial communication
    // Set the maximum speed in steps per second:
    stepper.setMaxSpeed(1000);
}

void loop()
{
    if (Serial.available() > 0)
    {
        char command = Serial.read(); // Read serial input

        // Check the command received
        if (command == 'A' || command == 'a')
        {
            // Start the motor
            stepper.setSpeed(400);
            motorRunning = true;
        }
        else if (command == 'C' || command == 'c')
        {
            // Stop the motor
            stepper.stop();
            motorRunning = false;
        }
    }

    // If motor is running, continue running it
    if (motorRunning)
    {
        stepper.runSpeed();
    }
}