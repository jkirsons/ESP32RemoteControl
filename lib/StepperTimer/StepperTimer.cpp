/*
 * Stepper.cpp - Stepper library for Wiring/Arduino - Version 1.1.0
 *
 * Original library        (0.1)   by Tom Igoe.
 * Two-wire modifications  (0.2)   by Sebastian Gassner
 * Combination version     (0.3)   by Tom Igoe and David Mellis
 * Bug fix for four-wire   (0.4)   by Tom Igoe, bug fix from Noah Shibley
 * 1-speed stepping mod         by Eugene Kozlenko
 * Timer rollover fix              by Eugene Kozlenko
 * Five phase five wire    (1.1.0) by Ryan Orendorff
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 * Drives a unipolar, bipolar, or five phase stepper motor.
 *
 * When wiring multiple stepper motors to a microcontroller, you quickly run
 * out of output pins, with each motor requiring 4 connections.
 *
 * By making use of the fact that at any time two of the four motor coils are
 * the inverse of the other two, the number of control connections can be
 * reduced from 4 to 2 for the unipolar and bipolar motors.
 *
 * A slightly modified circuit around a Darlington transistor array or an
 * L293 H-bridge connects to only 2 microcontroler pins, inverts the signals
 * received, and delivers the 4 (2 plus 2 inverted ones) output signals
 * required for driving a stepper motor. Similarly the Arduino motor shields
 * 2 direction pins may be used.
 *
 * The sequence of control signals for 4 control wires is as fol0s:
 *
 * Step C0 C1 C2 C3
 *    1  1  0  1  0
 *    2  0  1  1  0
 *    3  0  1  0  1
 *    4  1  0  0  1
 *
 * The circuits can be found at
 *
 * http://www.arduino.cc/en/Reference/Stepper
 */

#include "StepperTimer.h"

/*
 *   constructor for four-pin version
 *   Sets which wires should control the motor.
 */
StepperTimer::StepperTimer(int number_of_steps, timer_group_t group, timer_idx_t index, 
    int motor_pin_1, int motor_pin_2, int motor_pin_3, int motor_pin_4)
{
  this->step_number = 0;    // which step the motor is on
  this->number_of_steps = number_of_steps; // total number of steps for this motor

  // Arduino pins for the motor control connection:
  this->motor_pin_1 = motor_pin_1;
  this->motor_pin_2 = motor_pin_2;
  this->motor_pin_3 = motor_pin_3;
  this->motor_pin_4 = motor_pin_4;
  
  // setup the pins on the microcontroller:
  setPinMode(motor_pin_1, motor_pin_2, motor_pin_3, motor_pin_4);

  this->index = index;
  this->group = group;
  this->pin_count = 4;
}

void StepperTimer::setTargetSpeed(signed long whatSpeed)
{
  this->targetSpeed = whatSpeed;
  if(abs(this->speed) < 100)
  {
    setSpeed(whatSpeed);
  }
}

void StepperTimer::updateSpeed()
{
  if(this->speed < targetSpeed)
  {
    setSpeed(this->speed+1);
  }
  if(this->speed > targetSpeed)
  {
    setSpeed(this->speed-1);
  }
}

/*
 * Sets the speed in revs per minute
 */
void StepperTimer::setSpeed(signed long whatSpeed)
{
  this->speed = whatSpeed;
  if(whatSpeed != 0)
  {
    this->stepWaitTicks = (long)(((double)this->number_of_steps / (double)abs(whatSpeed)) * 3000.0);
    // Reset Alarm and set new value
    timer_set_alarm_value(this->group, this->index, this->stepWaitTicks);
    timer_set_alarm(this->group, this->index, TIMER_ALARM_EN);
  } else {
    this->stepWaitTicks = 8000UL;
  }
}

void StepperTimer::setMode(modeEnum mode)
{
  this->mode = mode;
}

void StepperTimer::step()
{
  if (this->speed == 0)
  {
     this->coast();
  } else {
    if (this->speed > 0)
    {
      this->step_number++;
      if (this->step_number >= this->number_of_steps * (this->mode == half ? 2 : 1)) {
        this->step_number = 0;
      }
    } else {
      if (this->step_number == 0) {
        this->step_number = this->number_of_steps * (this->mode == half ? 2 : 1);
      }
      this->step_number--;
    }
    this->stepMotor(this->step_number % (this->mode == half ? 8 : 4));
  }
}

void StepperTimer::disconnect()
{
  timer_disable_intr(this->group, this->index);
  timer_pause(this->group, this->index);
}

void StepperTimer::setPinMode(int motor_pin_1, int motor_pin_2, int motor_pin_3, int motor_pin_4)
{
  gpio_config_t io_conf;
  //disable interrupt
  io_conf.intr_type = GPIO_INTR_DISABLE;
  //set as output mode
  io_conf.mode = GPIO_MODE_OUTPUT;
  //bit mask of the pins that you want to set,e.g.all pins
  io_conf.pin_bit_mask = ((1ULL<<motor_pin_1)|(1ULL<<motor_pin_2)|(1ULL<<motor_pin_3)|(1ULL<<motor_pin_4));
  //disable pull-down mode
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  //disable pull-up mode
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  //configure GPIO with the given settings
  gpio_config(&io_conf);  
}

/*
 * Moves the motor forward or backwards.
 */
void StepperTimer::stepMotor(int thisStep)
{
  if(this->mode == half)
  {
    switch (thisStep) {
      case 0:
        gpio_set_level((gpio_num_t)motor_pin_1, 1);
        gpio_set_level((gpio_num_t)motor_pin_2, 0);
        gpio_set_level((gpio_num_t)motor_pin_3, 0);
        gpio_set_level((gpio_num_t)motor_pin_4, 1);
      break;
      case 1: 
        gpio_set_level((gpio_num_t)motor_pin_1, 1);
        gpio_set_level((gpio_num_t)motor_pin_2, 0);
        gpio_set_level((gpio_num_t)motor_pin_3, 0);
        gpio_set_level((gpio_num_t)motor_pin_4, 0);
      break;
      case 2: 
        gpio_set_level((gpio_num_t)motor_pin_1, 1);
        gpio_set_level((gpio_num_t)motor_pin_2, 0);
        gpio_set_level((gpio_num_t)motor_pin_3, 1);
        gpio_set_level((gpio_num_t)motor_pin_4, 0);
      break;
      case 3:
        gpio_set_level((gpio_num_t)motor_pin_1, 0);
        gpio_set_level((gpio_num_t)motor_pin_2, 0);
        gpio_set_level((gpio_num_t)motor_pin_3, 1);
        gpio_set_level((gpio_num_t)motor_pin_4, 0);
      break;
      case 4:
        gpio_set_level((gpio_num_t)motor_pin_1, 0);
        gpio_set_level((gpio_num_t)motor_pin_2, 1);
        gpio_set_level((gpio_num_t)motor_pin_3, 1);
        gpio_set_level((gpio_num_t)motor_pin_4, 0);
      break;
      case 5:  
        gpio_set_level((gpio_num_t)motor_pin_1, 0);
        gpio_set_level((gpio_num_t)motor_pin_2, 1);
        gpio_set_level((gpio_num_t)motor_pin_3, 0);
        gpio_set_level((gpio_num_t)motor_pin_4, 0);
      break;
      case 6: 
        gpio_set_level((gpio_num_t)motor_pin_1, 0);
        gpio_set_level((gpio_num_t)motor_pin_2, 1);
        gpio_set_level((gpio_num_t)motor_pin_3, 0);
        gpio_set_level((gpio_num_t)motor_pin_4, 1);
      break;
      case 7: 
        gpio_set_level((gpio_num_t)motor_pin_1, 0);
        gpio_set_level((gpio_num_t)motor_pin_2, 0);
        gpio_set_level((gpio_num_t)motor_pin_3, 0);
        gpio_set_level((gpio_num_t)motor_pin_4, 1);
      break;        
    }
  } else if(this->mode == full) {
    switch (thisStep) {
      case 0:  // 1010
        gpio_set_level((gpio_num_t)motor_pin_1, 1);
        gpio_set_level((gpio_num_t)motor_pin_2, 0);
        gpio_set_level((gpio_num_t)motor_pin_3, 1);
        gpio_set_level((gpio_num_t)motor_pin_4, 0);
      break;
      case 1:  // 0110
        gpio_set_level((gpio_num_t)motor_pin_1, 0);
        gpio_set_level((gpio_num_t)motor_pin_2, 1);
        gpio_set_level((gpio_num_t)motor_pin_3, 1);
        gpio_set_level((gpio_num_t)motor_pin_4, 0);
      break;
      case 2:  //0101
        gpio_set_level((gpio_num_t)motor_pin_1, 0);
        gpio_set_level((gpio_num_t)motor_pin_2, 1);
        gpio_set_level((gpio_num_t)motor_pin_3, 0);
        gpio_set_level((gpio_num_t)motor_pin_4, 1);
      break;
      case 3:  //1001
        gpio_set_level((gpio_num_t)motor_pin_1, 1);
        gpio_set_level((gpio_num_t)motor_pin_2, 0);
        gpio_set_level((gpio_num_t)motor_pin_3, 0);
        gpio_set_level((gpio_num_t)motor_pin_4, 1);
      break;
    }
  } else if(this->mode == wave) {
    switch (thisStep) {
      case 0: 
        gpio_set_level((gpio_num_t)motor_pin_1, 1);
        gpio_set_level((gpio_num_t)motor_pin_2, 0);
        gpio_set_level((gpio_num_t)motor_pin_3, 0);
        gpio_set_level((gpio_num_t)motor_pin_4, 0);
      break;
      case 1:
        gpio_set_level((gpio_num_t)motor_pin_1, 0);
        gpio_set_level((gpio_num_t)motor_pin_2, 0);
        gpio_set_level((gpio_num_t)motor_pin_3, 1);
        gpio_set_level((gpio_num_t)motor_pin_4, 0);
      break;
      case 2: 
        gpio_set_level((gpio_num_t)motor_pin_1, 0);
        gpio_set_level((gpio_num_t)motor_pin_2, 1);
        gpio_set_level((gpio_num_t)motor_pin_3, 0);
        gpio_set_level((gpio_num_t)motor_pin_4, 0);
      break;
      case 3: 
        gpio_set_level((gpio_num_t)motor_pin_1, 0);
        gpio_set_level((gpio_num_t)motor_pin_2, 0);
        gpio_set_level((gpio_num_t)motor_pin_3, 0);
        gpio_set_level((gpio_num_t)motor_pin_4, 1);
      break;
    }    
  }
}

void StepperTimer::coast()
{
    gpio_set_level((gpio_num_t)motor_pin_1, 0);
    gpio_set_level((gpio_num_t)motor_pin_2, 0);
    gpio_set_level((gpio_num_t)motor_pin_3, 0);
    gpio_set_level((gpio_num_t)motor_pin_4, 0);  
}
