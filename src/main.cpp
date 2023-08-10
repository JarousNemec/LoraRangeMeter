#include "main.h"

void setup() {
    Serial.begin(9600);

    //Init Transceiver
    ESerial.begin(9600);
    Serial.println("Starting Chat");
    Serial.println(Transceiver.init());
    Transceiver.PrintParameters();

    //Init web server and Wi-Fi ap
    randomSeed(analogRead(A0));
//    WiFi.softAP(ssid + String(random(300)), password);
    WiFi.softAP(ssid);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    delay(100);
    server.on("/", handle_OnConnect);
    server.onNotFound(handle_NotFound);
    server.begin();
    Serial.println("HTTP server started");

    //Load configuration
    LoadConfiguration();
}

void loop() {
    server.handleClient();
//    if (ESerial.available()) {
//        String input = ESerial.readString();
//        JsonObject &IncommingObj = jBuffer.parseObject(input);
//    }
}

String SendHTML() {
    String ptr = "<!DOCTYPE html>\n"
                 "<html lang=\"en\">\n"
                 "<head>\n"
                 "    <meta charset=\"UTF-8\">\n"
                 "    <title>Station control</title>\n"
                 "</head>\n"
                 "<body>\n"
                 "<h3>Actual set Id: " + String(StationConfig.Id) + "</h3>\n"
                                                                    "<h3>Actual set Type: " +
                 String(get_StationType_fromEnum())
                 +
                 "</h3><br>\n"
                 "<form action=\"/\" method=\"post\">\n"
                 "    <label for=\"id\">ID:</label>\n"
                 "    <input type=\"text\" id=\"id\" name=\"id\"><br>\n"
                 "    <label>Type:</label>\n"
                 "    <input type=\"radio\" id=\"master\" name=\"type\" value=\"Master\"><label for=\"master\">Master</label>\n"
                 "    <input type=\"radio\" id=\"slave\" name=\"type\" value=\"Slave\" checked=\"checked\"><label for=\"slave\">Slave</label><br>\n"
                 "    <input type=\"submit\" value=\"Submit\">\n"
                 "</form>\n"
                 "</body>\n"
                 "</html>";
    return ptr;
}

void handle_OnConnect() {

    if (server.method() == HTTP_POST && server.args() == 2) {
        if (server.argName(0) == "id" && server.argName(1) == "type") {
            server.arg(0).toCharArray(StationConfig.Id, 128);
            StationConfig.Type = get_StationType_fromString(server.arg(1));
            SaveConfiguration();
            Serial.println("Configuration writed.");
            LoadConfiguration();
        }
    }
    server.send(200, "text/html", SendHTML());
}

void handle_NotFound() {
    server.send(404, "text/plain", "Not found");
}

StationType get_StationType_fromString(const String &text) {
    if (text == "Slave") {
        return Slave;
    } else if (text == "Master") {
        return Master;
    } else if (text == "Bridge") {
        return Bridge;
    }
    return Undefined;
}

String get_StationType_fromEnum() {
    switch (StationConfig.Type) {
        case Master:
            return "Master";
        case Slave:
            return "Slave";
        case Bridge:
            return "Bridge";
        default:
            return "Undefined";
    }
}

void SaveConfiguration() {
    EEPROM.begin(512);
    EEPROM.put(CONFIGURATION_ADDRESS, StationConfig);
//    EEPROM.write(CONFIGURATION_ADDRESS, (byte) StationConfig.Id.length());
//    for (int i = 0; i < StationConfig.Id.length(); i++) {
//        EEPROM.write(CONFIGURATION_ADDRESS + i + 1, (byte) StationConfig.Id[i]);
//        Serial.print((byte) StationConfig.Id[i]);
//        Serial.print(" - ");
//        Serial.println(CONFIGURATION_ADDRESS + i + 1);
//    }
//
//    String type = get_StationType_fromEnum();
//    EEPROM.write(CONFIGURATION_ADDRESS + StationConfig.Id.length()+1, (byte) type.length());
//    for (int i = 0; i < type.length(); i++) {
//        EEPROM.write(CONFIGURATION_ADDRESS + StationConfig.Id.length() + i + 2, (byte) type[i]);
//        Serial.print((byte) type[i]);
//        Serial.print(" - ");
//        Serial.println(CONFIGURATION_ADDRESS + StationConfig.Id.length() + i + 2);
//    }
    boolean ok = EEPROM.commit();
    Serial.println((ok) ? "Commit OK" : "Commit failed");
    EEPROM.end();
}

void LoadConfiguration() {
    EEPROM.begin(512);
    EEPROM.get(CONFIGURATION_ADDRESS, StationConfig);
//    byte lenght = EEPROM.read(CONFIGURATION_ADDRESS);
//    char id[lenght];
//    for (byte i = 0; i < lenght; i++) {
//        id[i] = (char) EEPROM.read(CONFIGURATION_ADDRESS + i + 1);
//        Serial.print(id[i]);
//        Serial.print(" - ");
//        Serial.println(CONFIGURATION_ADDRESS + i + 1);
//    }
//    StationConfig.Id = String(id);
//
//
//    lenght = EEPROM.read(CONFIGURATION_ADDRESS + lenght+1);
//    char type[lenght];
//    for (int i = 0; i < lenght; i++) {
//        type[i]= (char)EEPROM.read(CONFIGURATION_ADDRESS + StationConfig.Id.length() + i + 2);
//        Serial.print(type[i]);
//        Serial.print(" - ");
//        Serial.println(CONFIGURATION_ADDRESS + StationConfig.Id.length() + i + 2);
//    }
//    StationConfig.Type = get_StationType_fromString(type);
    EEPROM.end();
    PrintConfig();
}

void PrintConfig() {
    Serial.println("Loaded configuration...");
    Serial.print("Id: ");
    Serial.println(StationConfig.Id);
    Serial.print("Type:");
    Serial.println(get_StationType_fromEnum());
    Serial.println("----------------");
}