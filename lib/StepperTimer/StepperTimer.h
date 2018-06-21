/*
 * Stepper.h - Stepper library for Wiring/Arduino - Version 1.1.0
 *
 * Original library        (0.1)   by Tom Igoe.
 * Two-wire modifications  (0.2)   by Sebastian Gassner
 * Combination version     (0.3)   by Tom Igoe and David Mellis
 * Bug fix for four-wire   (0.4)   by Tom Igoe, bug fix from Noah Shibley
 * High-speed stepping mod         by Eugene Kozlenko
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
 * The sequence of control signals for 4 control wires is as follows:
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

// ensure this library description is only included once
#ifndef StepperTimer_h
#define StepperTimer_h

#include "esp_attr.h"
#include "driver/timer.h"
#include "soc/timer_group_struct.h"
#include "driver/periph_ctrl.h"
#include "driver/gpio.h"
#include "esp_types.h"

#define TIMER_DIVIDER         16  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds

// library interface description
class StepperTimer {
  public:
    enum modeEnum { full, half, wave };

    // constructor:
    StepperTimer(int number_of_steps, timer_group_t group, timer_idx_t index, int motor_pin_1, int motor_pin_2,
                                 int motor_pin_3, int motor_pin_4);
    // speed setter method:
    void setSpeed(signed long whatSpeed);
    void updateSpeed();
    void setTargetSpeed(signed long whatSpeed);
    void disconnect();
    void step();
    void spin();
    void coast();
    void setMode(modeEnum mode);
    timer_idx_t index;
    timer_group_t group;

    signed long targetSpeed;
    int number_of_steps;      // total number of steps this motor can take
    unsigned long stepWaitTicks;
    int step_number;          // which step the motor is on
    void stepMotor(int this_step);
    signed long speed;
    modeEnum mode = full;

  private:
    int pin_count;            // how many pins are in use.

    // motor pin numbers:
    int motor_pin_1;
    int motor_pin_2;
    int motor_pin_3;
    int motor_pin_4;
    void setPinMode(int motor_pin_1, int motor_pin_2, int motor_pin_3, int motor_pin_4);


};

#endif