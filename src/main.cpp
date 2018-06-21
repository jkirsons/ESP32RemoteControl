/* 
 * ESP32 Web Based Motor Control
 *  by Jason Kirsons
 * 
 * Instructions:
 *  1 - Open this in Visual Studio Code
 *  2 - Download and install the PlatformIO plugin
 *  3 - Install the espressif32 Platform
 *  4 - Set your WiFi SSID and Password Below
 *  5 - Build (in the taskbar at the bottom)
 *  6 - In WebResponseImpl.h (ESP Async WebServer/src/WebResponseImpl.h)
 *       change line 62 to: #define TEMPLATE_PLACEHOLDER '@'
 *  7 - Plug in your Feather32 / TTGO Esp32
 *  8 - Upload (in the taskbar at the bottom)
 *  9 - Connect the the Serial Monitor (in the taskbar at the bottom)
 *  10 - Note the IP address of the ESP32, and connect to this with your browser
 * 
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */ 

// This project uses:
// https://github.com/me-no-dev/ESPAsyncWebServer

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "StepperTimer.h"
#include "DCMotorController.h"
#include <rom/rtc.h>
#include "pages.h"

using namespace std;

// Set your WiFi name & password here...
const char *ssid = "WiFiSSID";
const char *password = "Password123";

// for stepper motors
const int stepsPerRevolution = 200; // change this to fit the number of steps per revolution

#define feather  // comment this line out for Dev board configuration...
#ifdef feather
// Feather or TTGO ESP32 pin definitions
StepperTimer mySteppers[4] =
{
  StepperTimer(stepsPerRevolution, TIMER_GROUP_0, TIMER_0, 5, 4, 25, 26),
  StepperTimer(stepsPerRevolution, TIMER_GROUP_0, TIMER_1, 17, 16, 19, 18),
  StepperTimer(stepsPerRevolution, TIMER_GROUP_1, TIMER_0, 23, 22, 14, 32),
  StepperTimer(stepsPerRevolution, TIMER_GROUP_1, TIMER_1, 15, 33, 27, 12)
};

DCMotorController dcMotors[8] = 
{
  // Primaries (xA) - A channels
  DCMotorController(0, 5, 4), 
  DCMotorController(1, 17, 16), 
  DCMotorController(2, 23, 22), 
  DCMotorController(3, 15, 33), 
  // Secondaries (xB) - B channels
  DCMotorController(4, 25, 26), 
  DCMotorController(5, 19, 18), 
  DCMotorController(6, 14, 32), 
  DCMotorController(7, 27, 12)
};
#else
// Dev board - setup your own pins here...
StepperTimer mySteppers[4] =
{
  StepperTimer(stepsPerRevolution, TIMER_GROUP_0, TIMER_0, 16, 17, 22, 23),
  StepperTimer(stepsPerRevolution, TIMER_GROUP_0, TIMER_1, 1, 1, 1, 1),
  StepperTimer(stepsPerRevolution, TIMER_GROUP_1, TIMER_0, 1, 1, 1, 1),
  StepperTimer(stepsPerRevolution, TIMER_GROUP_1, TIMER_1, 1, 1, 1, 1)
};

DCMotorController dcMotors[8] = 
{
  // Primaries (xA) - A channels
  DCMotorController(0, 22, 23), 
  DCMotorController(1, 1, 1), 
  DCMotorController(2, 1, 1), 
  DCMotorController(3, 1, 1), 
  // Secondaries (xB) - B channels
  DCMotorController(4, 16, 17), 
  DCMotorController(5, 1, 1), 
  DCMotorController(6, 1, 1), 
  DCMotorController(7, 1, 1)
};
#endif

/* This is set from the Web Frontend...
 * 0 = off
 * 1 = stepper
 * 2 = dc brushed x2
 */
int channelMode[4] = {0, 0, 0, 0};
int motorSpeed[8] = {0,0,0,0,0,0,0,0};

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Timer Interrupt for Stepper Motors
void IRAM_ATTR timerInt(void *para)
{
  int index = (int)para;
  // Clear Timer
  if (mySteppers[index].group == TIMER_GROUP_0)
  {
    if (mySteppers[index].index == TIMER_0)
      TIMERG0.int_clr_timers.t0 = 1;
    else
      TIMERG0.int_clr_timers.t1 = 1;
  }
  else
  {
    if (mySteppers[index].index == TIMER_0)
      TIMERG1.int_clr_timers.t0 = 1;
    else
      TIMERG1.int_clr_timers.t1 = 1;
  }
  mySteppers[index].step();
  // Reset Timer
  if (mySteppers[index].speed != 0)
    timer_set_alarm(mySteppers[index].group, mySteppers[index].index, TIMER_ALARM_EN);
}

// Start spinning a Stepper Motor
static void spin(int index)
{
  timer_config_t config;
  config.divider = TIMER_DIVIDER;
  config.counter_dir = TIMER_COUNT_UP;
  config.counter_en = TIMER_PAUSE;
  config.alarm_en = TIMER_ALARM_EN;
  config.intr_type = TIMER_INTR_LEVEL;
  config.auto_reload = true;

  timer_init(mySteppers[index].group, mySteppers[index].index, &config);
  timer_set_counter_value(mySteppers[index].group, mySteppers[index].index, 0x00000000ULL);
  timer_set_alarm_value(mySteppers[index].group, mySteppers[index].index, mySteppers[index].stepWaitTicks);
  timer_set_auto_reload(mySteppers[index].group, mySteppers[index].index, TIMER_AUTORELOAD_EN);
  timer_enable_intr(mySteppers[index].group, mySteppers[index].index);
  timer_isr_register(mySteppers[index].group, mySteppers[index].index, timerInt, (void *)index, ESP_INTR_FLAG_IRAM, NULL);
  timer_start(mySteppers[index].group, mySteppers[index].index);
}

// replace %IP_ADDRESS% with the IP Address into html response
String processor(const String &var)
{
  if (var == "IP_ADDRESS")
    return WiFi.localIP().toString();
  return String();
}

// Handle WebSocket event
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (len)
  {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_BINARY)
    {
      unsigned int packetType = data[0];
      
      // control input packet
      if (packetType == 1)
      {
        for(int i = 0; i < 8; i++)
        {
          if(i*2+2 <= len)
          {
            motorSpeed[i] = (signed long)data[i*2+2] * ((signed long)data[i*2+1] - 1L);
            Serial.printf("%d: %d\n", i, motorSpeed[i]);
          } else {
            motorSpeed[i] = 0x00000000ULL;
          }
          if(i < 4 && channelMode[i] == 1)
          {
            mySteppers[i].setTargetSpeed(motorSpeed[i]);
            
          }
          if(channelMode[(i<4)?i:(i-4)] == 2)
          {
            dcMotors[i].SetSpeed(motorSpeed[i]);
          }
        }
      }

      //setup packet - channel
      if (packetType == 0)
      {
        for(int i = 0; i < 4; i++)
        {
          channelMode[i] = data[i+1];
          if(channelMode[i] == 1) {
            dcMotors[i].Disconnect();
            dcMotors[i+4].Disconnect();

            mySteppers[i].setMode(StepperTimer::modeEnum::full);
            mySteppers[i].setSpeed(0);
            spin(i);
          } if(channelMode[i] == 2) {
            mySteppers[i].disconnect();

            dcMotors[i].SetSpeed(0);
            dcMotors[i+4].SetSpeed(0);
          }
        }
      }
    }
  }
}

void setup()
{
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  // You can connect to the serial monitor to see the IP to connect to:
  Serial.println(WiFi.localIP());

  // attach AsyncWebSocket
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Handlers for HTML server requests
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", html, processor);
  });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/css", css);
  });

  server.on("/virt_joystick.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/javascript", virt_joystick);
  });

  server.on("/interact.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "application/javascript", interact, sizeof(interact));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/jquery.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "application/javascript", jquery, sizeof(jquery));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  // Start the server
  server.begin();

  // Turn on connected led
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop()
{
  // Gradually update the speed of stepper motors
  for(int i = 0; i < 4; i++)
    if(channelMode[i] == 1)
      mySteppers[i].updateSpeed();
  delay(5);
}