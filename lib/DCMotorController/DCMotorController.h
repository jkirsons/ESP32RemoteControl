

#ifndef DCMotorController_h
#define DCMotorController_h

#include "esp32-hal-ledc.h"
#include "esp32-hal-gpio.h"

// library interface description
class DCMotorController {
    public:
    DCMotorController(int channel, int pin1, int pin2);
    void SetSpeed(int speed);
    void SetBreaking(bool breaking);
    void Disconnect();

    private:
        int freq = 5000;
        int ledChannel = 0;
        int resolution = 8;
        int pin1 = 0;
        int pin2 = 1;
        int pwmPin = 0;

        bool breakMotor = false;

        int speed;
};

#endif