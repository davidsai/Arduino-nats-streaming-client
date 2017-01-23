#include <ESP8266WiFi.h>
#include "ArduinoNATS.h"

#include "StreamingNATS.h"

/* WIFI */
const char ssid[] = "Pi-AP";  //  your network SSID (name)
const char pass[] = "12345678";       // your network password
WiFiClient client;

/* StreamingNATS */
char nats_server[] = "172.24.1.141";
StreamingNATS* snats = new StreamingNATS(&client, nats_server);

/* Protobuff */

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

void setup() {
  // put your setup code here, to run once:
  delay(1000);
  Serial.begin(115200);

  wifiConnect();

  snats->connect();
  
  snats->publish("foo", "msg one", "_inbox.ack");
}

void loop() {
  snats->process();
}
