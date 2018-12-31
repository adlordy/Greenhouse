#define ARDUINO_ARCH_ESP8266
#include <Arduino.h>
#include "Nextion.h"
#include <ESP8266WiFi.h>
#include <NtpClientLib.h>
#include <EEPROM.h>

boolean wakeUp;
int timeZone = 1;
int minutesTimeZone = 0;
int MIN_VALUE = 25;
int MAX_VALUE = 1024;
int DARK_HOUR_START = 20;
int DARK_HOUR_STOP = 6;
int SENSOR_PIN = A0;
int LIGHT = D6;

NexScreen screen;
NexText dateText(0, 5, "dateText");

NexText airText(0, 8, "airText");
NexText groundText(0, 9, "groundText");
NexText waterText(0, 10, "waterText");
NexText airHumText(0, 11, "airHumText");
NexText groundHumText(0, 12, "groundHumText");
NexText luxText(0, 13, "luxText");
NexText currentTxt(1, 1, "currentText");
NexText targetTxt(1, 2, "targetText");

NexText *texts[] = {
    &airText,
    &groundText,
    &waterText,
    &airHumText,
    &groundHumText,
    &luxText};

NexButton buttons[] = {
    NexButton(1, 7, "applyBtn"),
    NexButton(1, 8, "cancelBtn"),
    NexButton(1, 5, "plusBtn"),
    NexButton(1, 6, "minusBtn")};

NexTouch *nex_listen_list[] = {
    &airText,
    &groundText,
    &waterText,
    &airHumText,
    &groundHumText,
    &luxText,
    &buttons[0],
    &buttons[1],
    &buttons[2],
    &buttons[3],
    NULL};

int current = -1, value = 0, target = 0;

int airValue = 20,
    groundValue = 21,
    waterValue = 22,
    airHumValue = 70,
    groundHumValue = 75,
    luxValue = 50;

int *values[] = {
    &airValue,
    &groundValue,
    &waterValue,
    &airHumValue,
    &groundHumValue,
    &luxValue};

int airTarget = 21,
    groundTarget = 20,
    waterTarget = 20,
    airHumTarget = 70,
    groundHumTarget = 75,
    luxTarget = 50;

int *targets[] = {
    &airTarget,
    &groundTarget,
    &waterTarget,
    &airHumTarget,
    &groundHumTarget,
    &luxTarget};

void setupNextion()
{
  nexInit();

  airText.attachPop(popCallback, &airText);
  groundText.attachPop(popCallback, &groundText);
  waterText.attachPop(popCallback, &waterText);
  airHumText.attachPop(popCallback, &airHumText);
  groundHumText.attachPop(popCallback, &groundHumText);
  luxText.attachPop(popCallback, &luxText);

  buttons[0].attachPop(apply);
  buttons[1].attachPop(cancel);
  buttons[2].attachPop(increase);
  buttons[3].attachPop(descrease);

  screen.setWakeCallback(wakeCallback);

  updateValues();
  //sendCommand("thsp=30");
  //sendCommand("thup=1");
}

void wakeCallback(unsigned short x, unsigned short y, byte event)
{
  if (event == 1)
  {
    wakeUp = true;
  }
}

void setupWiFi()
{
  WiFi.begin("nordline", "helloWorld");
  dateText.setText("Connecting...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
  }
  String status = "Connected. " + WiFi.localIP().toString();
  dateText.setText(status.c_str());
  NTP.begin("pool.ntp.org", timeZone, true, minutesTimeZone);
  NTP.setInterval(60);
}

void setupEEPROM()
{
  EEPROM.begin(512);
  for (int i = 0; i < 6; i++)
  {
    int address = sizeof(int) * i;
    EEPROM.get(address, *targets[i]);
    if (*targets[i] < 0 || *targets[i] > 100)
    {
      *targets[i] = 50;
    }
  }
}

void setup()
{
  pinMode(LIGHT, OUTPUT);
  digitalWrite(LIGHT, LOW);

  setupEEPROM();
  setupNextion();
  setupWiFi();
}

void popCallback(void *ptr)
{
  NexText *text = (NexText *)ptr;
  uint8_t id = text->getObjCid();
  for (current = 0; current < 6; current++)
  {
    if (id == texts[current]->getObjCid())
    {
      break;
    }
  }

  sendCommand("page 1");

  value = *values[current];
  target = *targets[current];

  update(value, currentTxt);
  update(target, targetTxt);
}

void updateTime(time_t t)
{
  dateText.setText(NTP.getTimeDateString(t).c_str());
}

void apply(void *ptr)
{
  sendCommand("page 0");
  *targets[current] = target;
  EEPROM.put(current*sizeof(int), target);
  EEPROM.commit();
  current = -1;
  updateValues();
}

void cancel(void *ptr)
{
  sendCommand("page 0");
  current = -1;
  updateValues();
}

void updateValues()
{
  update(airValue, airText);
  update(groundValue, groundText);
  update(waterValue, waterText);
  update(airHumValue, airHumText);
  update(groundHumValue, groundHumText);
  update(luxValue, luxText);
}

void update(int value, NexText text)
{
  String str = String(value);
  text.setText(str.c_str());
}

void increase(void *ptr)
{
  change(1);
}

void descrease(void *ptr)
{
  change(-1);
}

void change(int delta)
{
  target += delta;
  update(target, targetTxt);
}

void updateLux(time_t t)
{
  int sensorValue = analogRead(SENSOR_PIN);
  luxValue = min(100, max(0, sensorValue - MIN_VALUE) * 100 / (MAX_VALUE - MIN_VALUE));
  update(luxValue, luxText);
  if (current == 5)
  {
    update(luxValue, currentTxt);
  }

  int h = hour(t);
  if (h >= DARK_HOUR_START || h < DARK_HOUR_STOP)
  {
    digitalWrite(LIGHT, LOW);
  }
  else
  {
    if (luxValue < luxTarget)
    {
      digitalWrite(LIGHT, HIGH);
    }
    else
    {
      digitalWrite(LIGHT, LOW);
    }
  }
}

time_t last = 0;

void loop()
{
  nexLoop(nex_listen_list, &screen);

  time_t t = now();
  if (last != t)
  {
    updateTime(t);
    updateLux(t);
    last = t;
  }

  if (wakeUp)
  {
    updateValues();
    wakeUp = 0;
  }
  delay(100);
}
