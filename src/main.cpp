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
    server.on("/", HandleOnConnect);
    server.onNotFound(HandleNotFound);
    server.begin();
    Serial.println("HTTP server started");

    //Load configuration
    LoadConfiguration();
}

void loop() {
    server.handleClient();
    if (ESerial.available()) {
        String input = ESerial.readString();
        JsonObject &IncommingObj = jBuffer.parseObject(input);
        if (ValidatePacket(&IncommingObj)) {
            ProcessPacket(&IncommingObj);
        }
    }
    BehaveByStationType();
    jBuffer.clear();
}

void BehaveByStationType() {
    ValidateTimeouts();
    switch (StationConfig.Type) {
        case Beacon:
            break;
        case FieldStation:
            break;
        case Bridge:
            break;
        case Pinger:
            BehaveAsPinger();
            break;
        case Undefined:
            break;
    }
}

void ValidateTimeouts() {
    if (millis() - PingStatus.PingSentWhen > PING_TIMEOUT)
        PingStatus.PingSent = false;
    if (millis() - BeaconDiscoverStatus.DiscoverSentWhen > DISCOVER_TIMEOUT)
        BeaconDiscoverStatus.DiscoverSent = false;
}

void BehaveAsPinger() {
    if (BeaconId == "") {
        if (!BeaconDiscoverStatus.DiscoverSent) {
            Serial.println("Discover sent!");
            SendPacket(PacketType::Syn, "DiscB", StationConfig.Id, BeaconDiscoverStatus.DiscoverTarget);
            BeaconDiscoverStatus.DiscoverSent = true;
            BeaconDiscoverStatus.DiscoverSentWhen = millis();
        }
    } else {
        if (!PingStatus.PingSent) {
            Serial.println("Ping sent!");
            SendPacket(PacketType::Syn, "Ping", StationConfig.Id, BeaconId);
            PingStatus.PingSent = true;
            PingStatus.PingSentWhen = millis();
            PingStatus.PingSentTo = BeaconId;
        }
    }
}

void PrintPacketStructure(JsonObject *pckt) {
    Serial.print("Packet sent:");
    Serial.print((pckt->get<String>("t")));
    Serial.print(": ");
    Serial.print((pckt->get<String>("c")));
    Serial.print(": ");
    Serial.print((pckt->get<String>("s")));
    Serial.print(": ");
    Serial.println((pckt->get<String>("d")));
}

void SendPacket(PacketType type, String content, String source, String destination) {
    JsonObject &PacketToSend = jBuffer.createObject();
    PacketToSend["t"] = (int) type;
    PacketToSend["c"] = content;
    PacketToSend["s"] = source;
    PacketToSend["d"] = destination;
    String output;
    PacketToSend.printTo(output);
    ESerial.print(output);
    PrintPacketStructure(&PacketToSend);
}

void ProcessPacket(JsonObject *pckt) {
    String content = pckt->get<String>("c");
    if (content == "DiscB") {
        ProcessDiscoverBeacon(pckt);
    } else if (content == "Ping") {
        ProcessPing(pckt);
    }
}

void ProcessPing(JsonObject *pckt) {
    auto source = pckt->get<String>("s");
    auto destination = pckt->get<String>("d");
    auto type = (PacketType) pckt->get<int>("t");
    switch (type) {
        case Syn:
            if(destination == StationConfig.Id){
                SendPacket(PacketType::SynAck, "Ping", StationConfig.Id, source);
                PingStatus.PingSent = true;
                PingStatus.PingSentTo = source;
                PingStatus.PingSentWhen = millis();
            }
            break;
        case SynAck:
            if (PingStatus.PingSent && PingStatus.PingSentTo == source) {
                Serial.print("Successfully ping id:");
                Serial.println(source);
                PingStatus.PingSent = false;
                PingStatus.PingSentWhen = 0;
                PingStatus.PingSentTo = "all";
                SendPacket(PacketType::Ack, "Ping", StationConfig.Id, source);
            }
            break;
        case Ack:
            if (PingStatus.PingSent && PingStatus.PingSentTo == source) {
                Serial.print("Successfully ping id:");
                Serial.println(source);
                PingStatus.PingSent = false;
                PingStatus.PingSentWhen = 0;
                PingStatus.PingSentTo = "all";
            }
            break;
    }
}

void ProcessDiscoverBeacon(JsonObject *pckt) {
    auto source = pckt->get<String>("s");
    auto destination = pckt->get<String>("d");
    auto type = (PacketType) pckt->get<int>("t");
    switch (type) {
        case Syn:
            if (StationConfig.Type == Beacon) {
                SendPacket(PacketType::SynAck, "DiscB", StationConfig.Id, source);
                BeaconDiscoverStatus.DiscoverSent = true;
                BeaconDiscoverStatus.DiscoverTarget = source;
                BeaconDiscoverStatus.DiscoverSentWhen = millis();
            }
            break;
        case SynAck:
            if (BeaconDiscoverStatus.DiscoverSent && source != "" &&
                (StationConfig.Type == Pinger || StationConfig.Type == FieldStation)) {
                Serial.print("Successfully discovered beacon id:");
                Serial.println(source);
                BeaconId = source;
                BeaconDiscoverStatus.DiscoverSent = false;
                BeaconDiscoverStatus.DiscoverSentWhen = 0;
                BeaconDiscoverStatus.DiscoverTarget = "all";
                SendPacket(PacketType::Ack, "DiscB", StationConfig.Id, source);
            }
            break;
        case Ack:
            if (StationConfig.Type == Beacon && source == BeaconDiscoverStatus.DiscoverTarget &&
                BeaconDiscoverStatus.DiscoverSent) {
                Serial.println("Beacon successfully discovered new station!");
            }
            break;
    }
}

bool ValidatePacket(JsonObject *pckt) {
    return pckt->containsKey("t") && pckt->containsKey("s") && pckt->containsKey("d") && pckt->containsKey("c");
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
                 String(GetStationTypeFromEnum())
                 +
                 "</h3><br>\n"
                 "<form action=\"/\" method=\"post\">\n"
                 "    <label for=\"id\">ID:</label>\n"
                 "    <input type=\"text\" id=\"id\" name=\"id\" value=\""+String(StationConfig.Id)+"\"><br>\n"
                 "    <label>Type:</label>\n"
                 "    <input type=\"radio\" id=\"beacon\" name=\"type\" value=\"Beacon\"><label for=\"beacon\" "+(StationConfig.Type == Beacon ?"checked" :"")+" >Beacon</label>\n"
                 "    <input type=\"radio\" id=\"fieldStation\" name=\"type\" value=\"FieldStation\" "+(StationConfig.Type == FieldStation ?"checked" :"")+" ><label for=\"fieldStation\">FieldStation</label>\n"
                 "    <input type=\"radio\" id=\"bridge\" name=\"type\" value=\"Bridge\"><label for=\"bridge\" "+(StationConfig.Type == Bridge ?"checked" :"")+" >Bridge</label>\n"
                 "    <input type=\"radio\" id=\"pinger\" name=\"type\" value=\"Pinger\"><label for=\"pinger\" "+(StationConfig.Type == Pinger ?"checked" :"")+" >Pinger</label><br><br>\n"
                 "    <input type=\"submit\" value=\"Submit\">"
                 "</form>\n"
                 "</body>\n"
                 "</html>";
    return ptr;
}

void HandleOnConnect() {

    if (server.method() == HTTP_POST && server.args() == 2) {
        if (server.argName(0) == "id" && server.argName(1) == "type") {
            server.arg(0).toCharArray(StationConfig.Id, 128);
            StationConfig.Type = GetStationTypeFromString(server.arg(1));
            SaveConfiguration();
            Serial.println("Configuration writed.");
            LoadConfiguration();
        }
    }
    server.send(200, "text/html", SendHTML());
}

void HandleNotFound() {
    server.send(404, "text/plain", "Not found");
}


StationType GetStationTypeFromString(const String &text) {
    if (text == "FieldStation") {
        return FieldStation;
    } else if (text == "Beacon") {
        return Beacon;
    } else if (text == "Bridge") {
        return Bridge;
    } else if (text == "Pinger") {
        return Pinger;
    }
    return Undefined;
}

String GetStationTypeFromEnum() {
    switch (StationConfig.Type) {
        case Beacon:
            return "Beacon";
        case FieldStation:
            return "FieldStation";
        case Bridge:
            return "Bridge";
        case Pinger:
            return "Pinger";
        default:
            return "Undefined";
    }
}

void SaveConfiguration() {
    EEPROM.begin(512);
    EEPROM.put(CONFIGURATION_ADDRESS, StationConfig);
    boolean ok = EEPROM.commit();
    Serial.println((ok) ? "Commit OK" : "Commit failed");
    EEPROM.end();
}

void LoadConfiguration() {
    EEPROM.begin(512);
    EEPROM.get(CONFIGURATION_ADDRESS, StationConfig);
    EEPROM.end();
    PrintConfig();
}

void PrintConfig() {
    Serial.println("Loaded configuration...");
    Serial.print("Id: ");
    Serial.println(StationConfig.Id);
    Serial.print("Type:");
    Serial.println(GetStationTypeFromEnum());
    Serial.println("----------------");
}