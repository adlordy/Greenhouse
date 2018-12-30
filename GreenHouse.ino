#define ARDUINO_ARCH_ESP8266
#include "Nextion.h"
#include <ESP8266WiFi.h>
#include <NtpClientLib.h>
#include <Ticker.h>

boolean wakeUp;
int timeZone = 1;
int minutesTimeZone = 0;

Ticker timer;
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

NexText* texts[] = {
  &airText,
  &groundText,
  &waterText,
  &airHumText,
  &groundHumText,
  &luxText
};

NexButton buttons[] = {
  NexButton(1, 7, "applyBtn"),
  NexButton(1, 8, "cancelBtn"),
  NexButton(1, 5, "plusBtn"),
  NexButton(1, 6, "minusBtn")
};

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
  NULL
};

int current = 0, target = 0;

int airValue = 20, 
  groundValue = 21,
  waterValue = 22, 
  airHumValue = 70,
  groundHumValue = 75,
  luxValue = 50;

int* values[] = {
  &airValue,
  &groundValue,
  &waterValue,
  &airHumValue,
  &groundHumValue,
  &luxValue
};

void setupNextion(){
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
  sendCommand("thsp=30");
  sendCommand("thup=1");
}

void setupTicker(){
  //timer.attach(1, updateTime);
}

void wakeCallback(unsigned short x, unsigned short y, byte event){
  if (event == 1){
    wakeUp = true;
  }
}

void setupWiFi(){
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

void setup() {
  setupTicker();
  setupNextion();
  setupWiFi();
}

void popCallback(void *ptr)
{
    NexText *text = (NexText *) ptr;
    uint8_t id = text -> getObjCid();
    for(current = 0; current < 6; current++){
      if (id == texts[current]->getObjCid()){
        break;
      }
    }

    sendCommand("page 1");
    target = *values[current];

    update(target, currentTxt);
    update(target, targetTxt);
}

void updateTime(){
  long ms = millis();
  String text = String(ms / 1000);
  dateText.setText(text.c_str());
}

void apply(void *ptr){
  sendCommand("page 0");
  (*values[current]) = target;
  updateValues();
}

void cancel(void *ptr){
  sendCommand("page 0");
  updateValues();
}

void updateValues(){
  update(airValue, airText);
  update(groundValue, groundText);
  update(waterValue, waterText);
  update(airHumValue, airHumText);
  update(groundHumValue, groundHumText);
  update(luxValue, luxText);
}

void update(int value, NexText text){
  String str = String(value);
  text.setText(str.c_str());
}

void increase(void *ptr){
  change(1);
}

void descrease(void *ptr){
  change(-1);
}

void change(int delta){
  target += delta;
  update(target, targetTxt);
}

void loop() {
  nexLoop(nex_listen_list, &screen);

  dateText.setText(NTP.getTimeDateString().c_str());
  delay(1000);

  if (wakeUp){
    updateValues();
    delay(100);
    wakeUp = 0;
  }
}
