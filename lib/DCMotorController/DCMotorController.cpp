#include "DCMotorController.h"

DCMotorController::DCMotorController(int channel, int pin1, int pin2)
{
    this->ledChannel = channel;
    this->pin1 = pin1;
    this->pin2 = pin2;
    pinMode(this->pin1, OUTPUT);
    pinMode(this->pin2, OUTPUT);
    ledcSetup(this->ledChannel, this->freq, this->resolution);
    this->pwmPin = -1;
}

void DCMotorController::SetSpeed(int speed)
{
    this->speed = speed;
    if(speed > 0 && this->pwmPin != this->pin1){
        ledcDetachPin(this->pin2);
        digitalWrite(this->pin2, breakMotor?HIGH:LOW);
        ledcAttachPin(this->pin1, this->ledChannel);
        this->pwmPin = this->pin1;
    } else if(speed <= 0 && this->pwmPin != this->pin2){
        ledcDetachPin(this->pin1);
        digitalWrite(this->pin1, breakMotor?HIGH:LOW);
        ledcAttachPin(this->pin2, this->ledChannel);
        this->pwmPin = this->pin2;
    }
    ledcWrite(this->ledChannel, (speed < 0)?(255-speed):speed);    
}

void DCMotorController::SetBreaking(bool breaking)
{
    this->breakMotor = breaking;
}

void DCMotorController::Disconnect()
{
    ledcWrite(this->ledChannel, 0);
    ledcDetachPin(this->pin1); 
    ledcDetachPin(this->pin2);     
    digitalWrite(this->pin1, LOW);
    digitalWrite(this->pin2, LOW);  
}