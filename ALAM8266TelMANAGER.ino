#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <Ticker.h>

// Tus definiciones y variables aquí...

Ticker ticker;
byte pinLed = 4;

void parpadeoLed() {
  byte estado = digitalRead(pinLed);
  digitalWrite(pinLed, !estado);
}

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure client;



volatile bool door2StateChanged = false;
volatile bool pirStateChanged = false;

bool previousDoorState = false;
volatile bool doorStateChanged = false;
int botRequestDelay = 100;
unsigned long lastTimeBotRan;
const int puerta1 = 12;
const int puerta2 = 13;
const int PIR = 5;
const int buzz = 14;



const char* ssid = "Personal-250";
const char* password = "79D777E238";

#define BOTtoken "6605703445:AAGYnxji-z2lSjjiQc0EJZpn1IyqNMxkX4s"

//#define CHAT_ID "1172351395"
#define CHAT_ID "-4195292837"
UniversalTelegramBot bot(BOTtoken, client);


void IRAM_ATTR handleDoorInterrupt() {
  doorStateChanged = true;
}


void IRAM_ATTR handleDoor2Interrupt() {
  door2StateChanged = true;
}

void IRAM_ATTR handlePirInterrupt() {
  pirStateChanged = true;
}
bool alarmActive = true;

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID) {
      bot.sendMessage(chat_id, "USUARIO NO AUTORIZADO", "");
      continue;
    }

    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      String welcome = "BIENVENIDO, " + from_name + ".\n";
      welcome += "Este es un sistema de alarma que monitorea dos puertas y un sensor de movimiento PIR.\n";
      welcome += "Cuando la alarma está activada, cualquier cambio en el estado de los sensores se notificará en este chat.\n\n";
      welcome += "Puedes usar los siguientes comandos:\n";
      welcome += "/estado - Muestra el estado actual de todos los sensores.\n";
      welcome += "/prender - Activa la alarma.\n";
      welcome += "/apagar - Desactiva la alarma.\n";
      bot.sendMessage(chat_id, welcome, "");
    }

    if (text == "/estado") {
      String message = "Estado de los sensores:\n";
      message += "Puerta 1: " + String(digitalRead(puerta1) ? "Abierta" : "Cerrada") + "\n";
      message += "Puerta 2: " + String(digitalRead(puerta2) ? "Abierta" : "Cerrada") + "\n";
      message += "Sensor PIR: " + String(digitalRead(PIR) ? "Movimiento detectado" : "No hay movimiento") + "\n";
      message += "Estado de la alarma: " + String(alarmActive ? "Activada" : "Desactivada"); // Agrega el estado de la alarma
      bot.sendMessage(chat_id, message, "");
    }


    if (text == "/prender") {
      alarmActive = true;
      bot.sendMessage(chat_id, "Alarma activada.", "");
    }

    if (text == "/apagar") {
      alarmActive = false;
      bot.sendMessage(chat_id, "Alarma desactivada.", "");
    }
  }
}



void setup() {
  Serial.begin(9600);
#ifdef ESP8266
  configTime(0, 0, "pool.ntp.org");
  client.setTrustAnchors(&cert);
#endif

  pinMode(buzz, OUTPUT);

 
  pinMode(pinLed, OUTPUT);
  ticker.attach(0.2, parpadeoLed);
  pinMode(0, INPUT_PULLUP); // Configuramos el botón de flash como entrada

  pinMode(puerta1, INPUT);

  pinMode(puerta1, INPUT_PULLUP);
  pinMode(puerta2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(puerta1), handleDoorInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(puerta2), handleDoor2Interrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIR), handlePirInterrupt, CHANGE);

#ifdef ESP32
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
#endif
  WiFiManager wifiManager;
  if (!wifiManager.autoConnect("ESP8266Temp")) {
    Serial.println("Fallo en la conexión (timeout)");
    ESP.reset();
    delay(1000);
  }

  Serial.println("Ya estás conectado");
  ticker.detach();
  digitalWrite(pinLed, HIGH);
  String message = "Alarma INICIADA.\n";
  message += "Estado de la alarma: " + String(alarmActive ? "Activada" : "Desactivada") + "\n";
  message += "Estado de los sensores:\n";
  message += "Puerta casa: " + String(digitalRead(puerta1) ? "Abierta" : "Cerrada") + "\n";
  message += "Puerta baño: " + String(digitalRead(puerta2) ? "Abierta" : "Cerrada") + "\n";
  message += "Sensor PIR: " + String(digitalRead(PIR) ? "Movimiento detectado" : "No hay movimiento");
  bot.sendMessage(CHAT_ID, message, "");

}

void loop() {
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
    if (digitalRead(0) == LOW) {
    Serial.println("Botón de flash presionado. Reiniciando configuración WiFi...");
    delay(100); // Esperamos un breve instante para evitar rebotes
    WiFiManager wifiManager; // Creamos una instancia de la clase WiFiManager
    wifiManager.resetSettings(); // Reiniciamos la configuración WiFi
    ESP.restart(); // Reiniciamos el ESP8266
  }
  if (alarmActive) { // Solo comprueba los sensores si la alarma está activa
if (doorStateChanged) {
  digitalWrite(buzz, HIGH);
  delay(1000);
  digitalWrite(buzz, LOW);
  bool currentDoorState = digitalRead(puerta1); // No invierte el estado leído
  String message = String("🚪 PUERTA CASA 🏠 SE ") + String(currentDoorState ? "ABRIÓ 🔓" : "CERRÓ 🔒");
  bot.sendMessage(CHAT_ID, message, "");
  doorStateChanged = false;
}
if (door2StateChanged) {
  digitalWrite(buzz, HIGH);
  delay(1000);
  digitalWrite(buzz, LOW);
  bool currentDoorState = digitalRead(puerta2); // No invierte el estado leído
  String message = String("🚪 PUERTA BAÑO 🛁 SE ") + String(currentDoorState ? "ABRIÓ 🔓" : "CERRÓ 🔒");
  bot.sendMessage(CHAT_ID, message, "");
  door2StateChanged = false;
}





    if (pirStateChanged) {
      digitalWrite(buzz, HIGH);
      delay(1000);
      digitalWrite(buzz, LOW);
      bool currentPirState = digitalRead(PIR);
      String message = String("🏃 SENSOR DE MOVIMIENTO PIR DETECTÓ ") + String(currentPirState ? "MOVIMIENTO 🚨" : "FIN DE MOVIMIENTO 🆗");
      bot.sendMessage(CHAT_ID, message, "");
      pirStateChanged = false;
    }
  }
}
