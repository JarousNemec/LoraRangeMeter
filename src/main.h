#include <Arduino.h>
#include <SoftwareSerial.h>
#include "../lib/EBYTE/EBYTE.h"
#include "../lib/ArduinoJson/ArduinoJson.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "../lib/ESP_EEPROM/src/ESP_EEPROM.h"
//#include "EEPROM.h"

#ifndef LORARANGEMETER_MAIN_H
#define LORARANGEMETER_MAIN_H

#define PIN_RX 14   //D5 on the board (Connect this to the EBYTE TX pin)
#define PIN_TX 12   //D6 on the board (connect this to the EBYTE RX pin)

#define PIN_M0 5    //D1 on the board (connect this to the EBYTE M0 pin)
#define PIN_M1 4    //D2 on the board (connect this to the EBYTE M1 pin)
#define PIN_AX 16   //D0 on the board (connect this to the EBYTE AUX pin)
#define CONFIGURATION_ADDRESS 1
#define EEPROM_SIZE 512
// you will need to define the pins to create the serial port
SoftwareSerial ESerial(PIN_RX, PIN_TX);

// create the transceiver object, passing in the serial and pins
EBYTE Transceiver(&ESerial, PIN_M0, PIN_M1, PIN_AX);

DynamicJsonBuffer jBuffer;

enum PacketType {
    Syn, SynAck, Ack, GetReq, GetRes, SetReq, SetRes
};

enum StationType{
    Master, Slave, Bridge,Undefined
};

enum StationMode{
    Setup, Connecting, Running
};

struct ConfigStruct{
    char Id[128]{};
    StationType Type = Slave;
} StationConfig;
const String ssid = "LoraNet";  // Enter SSID here
const char *password = "12345678";  //Enter Password here

/* Put IP Address details */
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

ESP8266WebServer server(80);

String SendHTML();
void handle_NotFound();
void handle_OnConnect();
StationType get_StationType_fromString(const String& text);
String get_StationType_fromEnum();
void LoadConfiguration();
void PrintConfig();
void SaveConfiguration();

#endif //LORARANGEMETER_MAIN_H
