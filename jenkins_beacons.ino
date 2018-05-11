#include <RCSwitch.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

#include <PubSubClient.h>

RCSwitch mySwitch = RCSwitch();
String content = "";
String pulseLength = "";
String code = "";
String inString = "";
char character;
char character2;

//wifi configuration
WiFiClient wifiClient;
const char* host = "esp8266_01";
String esid  = "mySSID";
String epass = "mytkip";

// strings of topics for the mqtt broker
String subTopic="siren/team01/status";

//splunk http collector configuration
String collectorToken = "AF76112D-1111-1111-1111-101D66736836";
String eventData="";

//timer configuration in miliseconds
long interval = 420000;
long team1timer = 0;
int team1state = 0;

//mqtt broker ip adderess and creating a pub sub client for the broker to work with
IPAddress server(111, 22, 33, 44);
PubSubClient mqttClient(wifiClient);

char buf[40]; //For MQTT data recieve
//you need a different client per board otherwise the broker freaks out
String clientName ="03";
unsigned long currentTime;

void setup()
{
    Serial.begin(115200); // Reduce this if your Arduino has trouble talking so fast
    Serial.println("the beacons!");
    initWiFi();
    mySwitch.enableTransmit(13);  // Using Pin #13
    
    mqttClient.setServer(server, 1883);
    mqttClient.setCallback(mqtt_arrived);
}
//-------------------------------- Timer and power off function ---------------------------
void check_time()
{
    char* message = "off";
    int length = strlen(message);
    boolean retained = true;
    
    currentTime = millis();
    if (currentTime - team1timer > interval) {
        team1timer = currentTime;
        if (team1state == 1 ){
            rfsend("1off");
            delay(250);
            rfsend("2off");
            delay(250);
            rfsend("3off");
            delay(250);
            rfsend("4off");
            delay(250);
            rfsend("5off");
            team1state = 0;
            mqttClient.publish((char*)subTopic.c_str(),(byte*)message,length,retained);
            Serial.println("timer triggering 1off");
        }
    }
}

void rfsend(String code){
    if(code == "1off"){
        mySwitch.setPulseLength(185);
        mySwitch.send(267580, 24);
        mySwitch.setPulseLength(185);
        mySwitch.send(267580, 24);
        mySwitch.setPulseLength(185);
        mySwitch.send(267580, 24);
        mySwitch.setPulseLength(185);
        mySwitch.send(267580, 24);
    }
    if (code == "1on"){
        mySwitch.setPulseLength(185);
        mySwitch.send(267571, 24);
        mySwitch.setPulseLength(185);
        mySwitch.send(267571, 24);
        mySwitch.setPulseLength(185);
        mySwitch.send(267571, 24);
        mySwitch.setPulseLength(185);
        mySwitch.send(267571, 24);
    }
    
}

//--------------------------------initialize that wifi---------------------------
void initWiFi(){
    Serial.println();
    Serial.println();
    Serial.println("Startup");
    esid.trim();
    if ( esid.length() > 1 ) {
        // test esid
        WiFi.disconnect();
        delay(100);
        WiFi.mode(WIFI_STA);
        Serial.print("Connecting to WiFi ");
        Serial.println(esid);
        WiFi.begin(esid.c_str(), epass.c_str());
        if ( testWifi() == 20 ) {
            return;
        }
    }
    Serial.println("Opening AP");
}
//--------------------------------testing the wifi useful when you wait for an IP---------------------------

int testWifi(void) {
    int c = 0;
    Serial.println("Wifi test...");
    while ( c < 30 ) {
        if (WiFi.status() == WL_CONNECTED) { return(20); }
        delay(500);
        Serial.print(".");
        c++;
    }
    Serial.println("WiFi Connect timed out");
    return(10);
}

void loop()
{
    
    if (WiFi.status() != WL_CONNECTED) {
        initWiFi();
    }
    else{
        char          ipAddressString[16]; // 16 bytes needed
        unsigned long ipAddressLong;       // only 4 bytes needed
        if (!connectMQTT()){
            delay(200);
        }
        if (mqttClient.connected()){
            mqtt_handler();
        }
        check_time();
    }
}



String macToStr(const uint8_t* mac)
{
    String result;
    for (int i = 0; i < 6; ++i) {
        result += String(mac[i], 16);
        if (i < 5)
            result += ':';
    }
    return result;
}



//-------------------------------- MQTT functions ---------------------------
boolean connectMQTT(){
    if (mqttClient.connected()){
        return true;
    }
    
    uint8_t mac[6];
    WiFi.macAddress(mac);
    
    Serial.print("Connecting to MQTT server ");
    Serial.print(String(server));
    Serial.print(" as ");
    Serial.println(clientName.c_str());
    eventData="clientname=";
    eventData=eventData+clientName.c_str();
    eventData=" Connect_MQTT_server_ ";
    splunkpost(collectorToken,eventData,clientName);
    
    Serial.println("");
    Serial.println(eventData);
    
    //if (mqttClient.connect((char*) clientName.c_str())) {
    if (mqttClient.connect((char*) clientName.c_str())) {
        Serial.println("Connected to MQTT broker");
        if(mqttClient.subscribe((char*)subTopic.c_str())){
            Serial.println("Subsribed to topic.");
            Serial.print((char*)subTopic.c_str());
            Serial.println("");
            eventData="clientname=";
            eventData=eventData+clientName.c_str();
            eventData=eventData+" topic_subscribed="+(char*)subTopic.c_str();
            splunkpost(collectorToken,eventData,clientName);
        } else {
            Serial.println("NOT subsribed to topic!");
            Serial.print((char*)subTopic.c_str());
            Serial.println("");
            eventData="clientname=";
            eventData=eventData+clientName.c_str();
            eventData=eventData+"f ailed_topic_subscribed="+(char*)subTopic.c_str();
            splunkpost(collectorToken,eventData,clientName);
        }
        return true;
    }
    else {
        Serial.println("MQTT connect failed! ");
        return false;
        eventData="FAILED_Connect_MQTT_server_";
        eventData=eventData+clientName.c_str();
        splunkpost(collectorToken,eventData,clientName);
    }
}

void disconnectMQTT(){
    mqttClient.disconnect();
}

void mqtt_handler(){
    if (toPub==1){
        if(pubState()){
            toPub=0;
        }
    }
    mqttClient.loop();
    delay(100); //let things happen in background
}

void mqtt_arrived(char* subTopic, byte* payload, unsigned int length) { // handle messages arrived
    int i = 0;
    Serial.print(String(subTopic));
    // create character buffer with ending null terminator (string)
    for(i=0; i<length; i++) {
        buf[i] = payload[i];
    }
    buf[i] = '\0';
    String msgString = String(buf);
    Serial.println(" " +msgString);
    eventData="clientname=";
    eventData=eventData+clientName.c_str();
    eventData=eventData+" topic="+String(subTopic);
    eventData=eventData+" message_recieved="+String(msgString);
    splunkpost(collectorToken,eventData,clientName);
    
    if (String(subTopic) == "siren/team01/status")
    {
        if (String(msgString) == "on")
        {
            rfsend("1on");
            delay(250);
            rfsend("2on");
            delay(250);
            rfsend("3on");
            delay(250);
            rfsend("4on");
            delay(250);
            rfsend("5on");
            team1timer = currentTime;
            team1state=1;
            
        }
        
        if (String(msgString) == "off")
        {
            rfsend("1off");
            delay(250);
            rfsend("2off");
            delay(250);
            rfsend("3off");
            delay(250);
            rfsend("4off");
            delay(250);
            rfsend("5off");
            team1state = 0;
        }
    }
    
    
}

boolean pubState(){ //Publish the current state of the light
    if (!connectMQTT()){
        delay(100);
        if (!connectMQTT){
            Serial.println("Could not connect MQTT.");
            Serial.println("Publish state NOK");
            return false;
        }
    }
}

//-------------------------------Splunk all the THINGS!--------------------------
void splunkpost(String collectorToken,String PostData, String Host)
{
    
    String payload = "{ \"host\" : \"" + Host +"\", \"sourcetype\" : \"httpevent\", \"index\" : \"main\", \"event\": {\"data\" : \"" + PostData + "\"}}";
    
    HTTPClient http;
    
    //splunk
    http.begin("http://111.22.33.44:8088/services/collector");
    http.addHeader("Content-Type", "application/json");
    String tokenValue= "Splunk " + collectorToken;
    http.addHeader("Authorization", "Splunk AF76112D-1111-1111-1111-101D66736836");
    String contentlength = String(payload.length());
    http.addHeader("Content-Length", contentlength );
    http.POST(payload);
    http.writeToStream(&Serial);
    http.end();
    Serial.println("");
    
}
