#ifndef StreamingNATS_H
#define StreamingNATS_H

// the #include statment and code go here...
#include <ESP8266WiFi.h>
#include "pb_encode.h"
#include "pb_decode.h"
#include "protocol.pb.h"
#include "ArduinoNATS.h"


class StreamingNATS {

// Structures/Types ---------------------------------------------//

private:

  typedef enum {
    Msg_ConnectRequest = 0,
    Msg_ConnectResponse = 1,
    Msg_SubscriptionRequest = 2,
    Msg_SubscriptionResponse = 3,
    Msg_UnsubscribeRequest = 4,
    Msg_PubMsg = 5,
    Msg_PubAck = 6,
    Msg_MsgProto = 7,
    Msg_Ack = 8,
    Msg_CloseRequest = 9,
    Msg_CloseResp = 10
  } Message;

  typedef unsigned char BYTE;

// Members -------------------------------------------------//

private:

  NATS* basicNats;
  static ConnectResponse connRes;

public:

  bool connected;

// Constructor ---------------------------------------------//

public:
    StreamingNATS(Client* client, const char* hostname,
        int port = NATS_DEFAULT_PORT) :
      basicNats(new NATS(client, hostname, port)),
      connected(false) {}

// Methods ----------------------------------------------//

private:

  BYTE* buildMessage(Message msg, ...) {
    va_list args;
    va_start(args, msg);

    switch(msg) {
      case Msg_ConnectRequest:
        ConnectRequest m = ConnectRequest_init_zero;

        strcpy(m.clientID, va_arg(args, char*));
        strcpy(m.heartbeatInbox, va_arg(args, char*));
        va_end(args);

        BYTE* buff = (BYTE*)calloc(1,sizeof(m));
        pb_ostream_t stream = pb_ostream_from_buffer(buff, sizeof(m)+1);
        pb_encode(&stream, ConnectRequest_fields, &m);
        return buff;
    }
  }

  static void storeRes(NATS::msg msg) {
    ConnectResponse m = ConnectResponse_init_zero;
    pb_istream_t stream = pb_istream_from_buffer((unsigned char *)msg.data, msg.size);
    pb_decode(&stream, ConnectResponse_fields, &m);
    connRes = m;
    Serial.println(connRes.pubPrefix);
  }

  void processAll() {
    delay(1000);
    while(basicNats->client->available()) basicNats->process();
  }

public:

  void process() {
    basicNats->process();
  }

  bool connect() {
    //connect to nats server by basicNats
    if (!basicNats->connected) {
      if (!basicNats->connect()) {
        Serial.println("Connection to nats server failed");
        return false;
      }
    }
    Serial.println("Connection to nats server succeeded!");
    processAll();
    // ConnectRequest
    BYTE* buff= buildMessage(Msg_ConnectRequest, "123", "inbox");
    basicNats->request("_STAN.discover.test-cluster", (char*)buff, storeRes);
    free(buff);
  }

};


#endif
