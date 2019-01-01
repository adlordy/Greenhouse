#define ARDUINO_ARCH_ESP8266
#include <Arduino.h>
#include "Nextion.h"
#include <ESP8266WiFi.h>
#include <NtpClientLib.h>
#include <EEPROM.h>

boolean wakeUp; // флаг пробуждения монитора

int timeZone = 1; // часов пояс в часах от UTC
int minutesTimeZone = 0; // часовой пояс в минутах от UTC

int MIN_VALUE = 25;  // минимальный предел для датчика освещенности
int MAX_VALUE = 1024; // максимальный предел для датчика освещенности

int DARK_HOUR_START = 20; // время выключения осветительного оборудования
int DARK_HOUR_STOP = 6; // время включения осветительного оборудования

int SENSOR_PIN = A0; // пин датчика освещенности
int LIGHT = D6; // пин включения осветительного оборудования

NexScreen screen; // объект для работы с сенсорным экраном
// компоненты экрана на первой странице
NexText dateText(0, 5, "dateText"); 
NexText airText(0, 8, "airText");
NexText groundText(0, 9, "groundText");
NexText waterText(0, 10, "waterText");
NexText airHumText(0, 11, "airHumText");
NexText groundHumText(0, 12, "groundHumText");
NexText luxText(0, 13, "luxText");
// компоненты кнопок на второй странице
NexText currentTxt(1, 1, "currentText");
NexText targetTxt(1, 2, "targetText");
// массив указателей на компоненты с отображаемыми значениями, реагирующие на нажатие
NexText *texts[] = {
    &airText,
    &groundText,
    &waterText,
    &airHumText,
    &groundHumText,
    &luxText
};
// кнопки на второй странице
NexButton applyBtn(1, 7, "applyBtn");
NexButton cancelBtn(1, 8, "cancelBtn");
NexButton plusBtn(1, 5, "plusBtn");
NexButton minusBtn(1, 6, "minusBtn");
// массив ссылок на компоненты реагирующие на нажание
NexTouch *nex_listen_list[] = {
    &airText,
    &groundText,
    &waterText,
    &airHumText,
    &groundHumText,
    &luxText,
    &applyBtn,
    &cancelBtn,
    &plusBtn,
    &minusBtn,
    NULL
};

int current = -1, // индекс текущего выбранного показателя для второй страницы
    value = 0, // текущие значение показателя на второй странице
    target = 0; // целевое значение показателя на второй странице
// измеряемые параметры
int airValue = 0,
    groundValue = 0,
    waterValue = 0,
    airHumValue = 0,
    groundHumValue = 0,
    luxValue = 0;
// ссылки на измеряемые показатели
int *values[] = {
    &airValue,
    &groundValue,
    &waterValue,
    &airHumValue,
    &groundHumValue,
    &luxValue
};
// целевые показатели
int airTarget = 20,
    groundTarget = 20,
    waterTarget = 20,
    airHumTarget = 70,
    groundHumTarget = 75,
    luxTarget = 50;
// ссылки на целевые показатели
int *targets[] = {
    &airTarget,
    &groundTarget,
    &waterTarget,
    &airHumTarget,
    &groundHumTarget,
    &luxTarget
};

// настройка компонентов сенсорной панели
void setupNextion()
{
  // инициализация библиотеки
  nexInit();
  // вызвать функцию при просыпании экрана
  screen.setWakeCallback(wakeCallback);
  // обработчик нажатия на сенсорном экране на значения показателей
  // вызывается функция popCallback с ссылкой на каждый из компонент

  airText.attachPop(airCallback);
  groundText.attachPop(groundCallback);
  waterText.attachPop(waterCallback);
  airHumText.attachPop(airHumCallback);
  groundHumText.attachPop(groundHumCallback);
  luxText.attachPop(luxCallback);
  // регистрация обработчиков нажания на кнопки
  applyBtn.attachPop(apply);
  cancelBtn.attachPop(cancel);
  plusBtn.attachPop(increase);
  minusBtn.attachPop(descrease);
  // обновить экран с текущими показателями
  updateValues();
  // параметры для перехода в спящий режим
  //sendCommand("thsp=30"); // время переключения экрана в спящий режим в секундах
  sendCommand("thup=1"); // флаг=1 - просыпание при нажании, флаг=0 - не просыпается
}
// обработчик при просыпании
void wakeCallback(unsigned short x, unsigned short y, byte event)
{
  if (event == 1)
  {
    wakeUp = true;
  }
}
// настрока WI-FI
void setupWiFi()
{
  // название сети и пароль
  WiFi.begin("nordline", "helloWorld");

  dateText.setText("Connecting...");
  // ждем подключения к сети
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
  }
  
  IPAddress ip = WiFi.localIP();
  String status = "Connected. " + ip.toString();
  // выводим IP адрес
  dateText.setText(status.c_str());
  // инициализация библиотеки NTP для получения текущего времени из интернета
  NTP.begin("pool.ntp.org", timeZone, true, minutesTimeZone);
  // период синхронизации
  NTP.setInterval(60);
}

// читаем целевые значения из энергонезавизимой памяти EEPROM
void setupEEPROM()
{
  EEPROM.begin(512);
  for (int i = 0; i < 6; i++)
  {
    int address = sizeof(int) * i;
    EEPROM.get(address, *targets[i]);
    // проверяем правльность значений
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

// выставляем значение переменной current в нужное значение в зависимости от нажатой кнопки
void airCallback(void *p){
  current = 0;
  popCallback();
}

void groundCallback(void *p){
  current = 1;
  popCallback();
}

void waterCallback(void *p){
  current = 2;
  popCallback();
}

void airHumCallback(void *p){
  current = 3;
  popCallback();
}

void groundHumCallback(void *p){
  current = 4;
  popCallback();
}

void luxCallback(void *p){
  current = 5;
  popCallback();
}

// общий обработчик нажания на показатель
void popCallback()
{
  // переход на вторую страницу
  sendCommand("page 1");
  // получение значений из соотвествующих массивов
  value = *values[current];
  target = *targets[current];
  // обновление экрана
  update(value, currentTxt);
  update(target, targetTxt);
}
// выводим время на экран
void updateTime(time_t t)
{
  dateText.setText(NTP.getTimeDateString(t).c_str());
}
// применяем изменения целевого значения настраиваемого параметра
void apply(void *ptr)
{
  // сохранение изменения в массив целевых значений
  *targets[current] = target;
  // запись в энергонезавизимую память
  EEPROM.put(current*sizeof(int), target);
  EEPROM.commit();

  cancel(NULL);
}

// сброс без примения изменений
void cancel(void *ptr)
{
  sendCommand("page 0");

  current = -1;
  updateValues();
}

void change(int delta)
{
  target += delta;
  update(target, targetTxt);
}

void increase(void *ptr)
{
  change(1);
}

void descrease(void *ptr)
{
  change(-1);
}

// обновние первой страницы экрана значениями изменений
void updateValues()
{
  update(airValue, airText);
  update(groundValue, groundText);
  update(waterValue, waterText);
  update(airHumValue, airHumText);
  update(groundHumValue, groundHumText);
  update(luxValue, luxText);
}

// обновить компонент числовым значением
void update(int value, NexText text)
{
  String str = String(value);
  text.setText(str.c_str());
}
// обработчик освещенности помещения
void readLuxSensor(time_t t)
{
  // считываем и нормализуем покания датчика освещенности
  int sensorValue = analogRead(SENSOR_PIN);
  luxValue = min(100, max(0, sensorValue - MIN_VALUE) * 100 / (MAX_VALUE - MIN_VALUE));
  
  update(luxValue, luxText);
  // обновления текущего значения на второй странице, если выбран 
  if (current == 5)
  {
    update(luxValue, currentTxt);
  }
  // проверяем час
  int h = hour(t);
  if (h >= DARK_HOUR_START || h < DARK_HOUR_STOP)
  {
    digitalWrite(LIGHT, LOW);
  }
  else
  {
    // проверяем целевое значение
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
  // вызов библиотеки сенсорного экрана
  nexLoop(nex_listen_list, &screen);

  // вызываем обновления времени и сенсоров только раз в секунду
  time_t t = now();
  if (last != t)
  {
    updateTime(t);
    readLuxSensor(t);
    last = t;
  }

  if (wakeUp)
  {
    updateValues(); // обновить значение показателей на экране
    wakeUp = false;
  }
  
  delay(100);
}
