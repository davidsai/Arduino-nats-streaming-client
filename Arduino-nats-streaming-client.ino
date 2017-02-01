#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "StreamingNATS.h"
#include <Adafruit_ADXL345_U.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

/* Remote deployment */
const char* host = "esp8266-webupdate";
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

/* WIFI */
const char ssid[] = "Pi-AP";  //  your network SSID (name)
const char pass[] = "12345678";       // your network password
WiFiClient client;

/* NTP */
int timeZone = 11;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "au.pool.ntp.org", 3600*timeZone);
uint64_t launchTime = 0;
char tsBuffer[20] = {};

/* NATS */
char nats_server[] = "10.155.156.251";
int port = 4222;
StreamingNATS* snats = new StreamingNATS(&client,nats_server,port);

/* Sensor */
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(1);
char sensorId[] = "test";
range_t range = ADXL345_RANGE_2_G;
dataRate_t dataRate = ADXL345_DATARATE_400_HZ;

/* Packet parameters */
const int packetSize = 15; //80
const int sampleRate = 300; //300
const int overhead = 260;
const int packetBufferSize = overhead+packetSize*42+10;
unsigned int interval = (1.0/sampleRate)*1000000;
char packet[packetBufferSize] = {};

void wifiConnect() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  for (int i=0; WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    Serial.print(".");
    if (i>10){
      WiFi.disconnect();
      WiFi.begin(ssid, pass);
      i = 0;
    }
  }
  Serial.printf("\nSuccessfully connected to wifi access point %s\n", ssid);
}

void getCurTime() {
  uint64_t t = micros() + launchTime;
//  Serial.println(t/);
  sprintf(tsBuffer, "%lu.%06lu", (uint32_t)(t/1000000), (uint32_t)(t%1000000));
}

void getPacket(char *packet) {
  /* JSON buffer */
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
 

  root["packetType"] = "accRaw";
  root["sensorId"] = sensorId;
  root["streamId"] = "";

  JsonArray& values = root.createNestedArray("values");
  
  getCurTime();
  root["startTime"] = jsonBuffer.strdup(tsBuffer);

  for (int i = 0; i < packetSize; i++) {
    JsonArray& value = jsonBuffer.createArray();
    getCurTime();
    double x = accel.getX()*ADXL345_MG2G_MULTIPLIER;
    double y = accel.getY()*ADXL345_MG2G_MULTIPLIER;
    double z = accel.getZ()*ADXL345_MG2G_MULTIPLIER;

    value.add(jsonBuffer.strdup(tsBuffer));
    value.add(double_with_n_digits(x, 3));
    value.add(double_with_n_digits(y, 3));
    value.add(double_with_n_digits(z, 3));

    values.add(value);
    delayMicroseconds(interval);
  }

  getCurTime();
  root["endTime"] = jsonBuffer.strdup(tsBuffer);

  JsonArray& columns = root.createNestedArray("columns");
  JsonObject& ts = jsonBuffer.createObject();ts["name"] = "time";ts["type"] = "ts";columns.add(ts);
  JsonObject& x = jsonBuffer.createObject();x["name"] = "x";x["type"] = "acc_g";columns.add(x);
  JsonObject& y = jsonBuffer.createObject();y["name"] = "y";y["type"] = "acc_g";columns.add(y);
  JsonObject& z = jsonBuffer.createObject();z["name"] = "z";z["type"] = "acc_g";columns.add(z);

  root.printTo(packet,packetBufferSize);
}

void natsConnect() {
//  nats->on_connect = nats_on_connect;
  if (!snats->connect()) {
    Serial.println("connection to nats server fails");
    return;
  }
  Serial.printf("Successfully connected to nats server %s\n", nats_server);
}

//void change_nats_server(NATS::msg msg) {  
//  Serial.printf("Switching to server at %s\n", msg.data);
//  
//  if(nats != NULL) { nats->disconnect(); delete nats; }
//  nats = new NATS(
//    &client,
//    msg.data,NATS_DEFAULT_PORT
//  );
//  natsConnect();
//}

//void nats_on_connect() {
//  nats->subscribe("change_nats_server", change_nats_server);
//  Serial.println("Publishing...");
//}


void initSensor() {
  if(!accel.begin())
  {
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
    while(1);
  }

  /* Set the range and data rate */
  //Note that the FIFO queue is not used
  accel.setRange(range);
  accel.setDataRate(dataRate);
}

void displayDataRate(void)
{
  Serial.print  ("Data Rate:    ");

  switch(accel.getDataRate())
  {
    case ADXL345_DATARATE_3200_HZ:
      Serial.print  ("3200 ");
      break;
    case ADXL345_DATARATE_1600_HZ:
      Serial.print  ("1600 ");
      break;
    case ADXL345_DATARATE_800_HZ:
      Serial.print  ("800 ");
      break;
    case ADXL345_DATARATE_400_HZ:
      Serial.print  ("400 ");
      break;
    case ADXL345_DATARATE_200_HZ:
      Serial.print  ("200 ");
      break;
    case ADXL345_DATARATE_100_HZ:
      Serial.print  ("100 ");
      break;
    case ADXL345_DATARATE_50_HZ:
      Serial.print  ("50 ");
      break;
    case ADXL345_DATARATE_25_HZ:
      Serial.print  ("25 ");
      break;
    case ADXL345_DATARATE_12_5_HZ:
      Serial.print  ("12.5 ");
      break;
    case ADXL345_DATARATE_6_25HZ:
      Serial.print  ("6.25 ");
      break;
    case ADXL345_DATARATE_3_13_HZ:
      Serial.print  ("3.13 ");
      break;
    case ADXL345_DATARATE_1_56_HZ:
      Serial.print  ("1.56 ");
      break;
    case ADXL345_DATARATE_0_78_HZ:
      Serial.print  ("0.78 ");
      break;
    case ADXL345_DATARATE_0_39_HZ:
      Serial.print  ("0.39 ");
      break;
    case ADXL345_DATARATE_0_20_HZ:
      Serial.print  ("0.20 ");
      break;
    case ADXL345_DATARATE_0_10_HZ:
      Serial.print  ("0.10 ");
      break;
    default:
      Serial.print  ("???? ");
      break;
  }
  Serial.println(" Hz");
}

void displayRange(void)
{
  Serial.print  ("Range:         +/- ");

  switch(accel.getRange())
  {
    case ADXL345_RANGE_16_G:
      Serial.print  ("16 ");
      break;
    case ADXL345_RANGE_8_G:
      Serial.print  ("8 ");
      break;
    case ADXL345_RANGE_4_G:
      Serial.print  ("4 ");
      break;
    case ADXL345_RANGE_2_G:
      Serial.print  ("2 ");
      break;
    default:
      Serial.print  ("?? ");
      break;
  }
  Serial.println(" g");
}

//void initJsonBuffer() {
//  root["packetType"] = "accRaw";
//  root["sensorId"] = sensorId;
//  root["streamId"] = "";
//
//  JsonArray& columns = root.createNestedArray("columns");
//  JsonObject& ts = jsonBuffer.createObject();ts["name"] = "time";ts["type"] = "ts";columns.add(ts);
//  JsonObject& x = jsonBuffer.createObject();x["name"] = "x";x["type"] = "acc_g";columns.add(x);
//  JsonObject& y = jsonBuffer.createObject();y["name"] = "y";y["type"] = "acc_g";columns.add(y);
//  JsonObject& z = jsonBuffer.createObject();z["name"] = "z";z["type"] = "acc_g";columns.add(z);
//
//  value.add(0);value.add(0);value.add(0);value.add(0);
//
//  for (int i = 0; i < packetSize; i++) {
//    values.add(0);
//  }
//}

void initNTP() {
  timeClient.begin();
  while (!timeClient.forceUpdate()) delay(100);
}

byte mac[6];

void printMac() {
  Serial.print(mac[5], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.println(mac[0], HEX);
}

void setup(){
  delay(1000);
  Serial.begin(115200);

  wifiConnect();

  initNTP();
  launchTime = ((uint64_t)timeClient.getEpochTime())*1000000-micros();

  initSensor();
  displayDataRate();
  displayRange();

//  initJsonBuffer();

  MDNS.begin(host);
  httpUpdater.setup(&httpServer);
  httpServer.begin();
  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready!\n");
  Serial.println(WiFi.localIP());
  WiFi.macAddress(mac);
  printMac();

  natsConnect();

  //ESP.wdtDisable();

}

void loop() {
  /* Compulsory */
  timeClient.update();
  httpServer.handleClient();
  yield();

  if(snats!=NULL) {
    snats->process();
     snats->process();
      snats->process();
       snats->process();
        snats->process();
    getPacket(packet);
    snats->publish(sensorId,packet,"_inbox.ack");
  }

//  if(WiFi.status() != WL_CONNECTED) {Serial.println("WiFi connection lost, attempt to re/connect...");/wifiConnect();natsConnect();}
}

