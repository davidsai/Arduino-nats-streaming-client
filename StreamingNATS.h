#ifndef StreamingNATS_H
#define StreamingNATS_H

// the #include statment and code go here...
#include <ESP8266WiFi.h>
#include "pb_encode.h"
#include "pb_decode.h"
#include "protocol.pb.h"
#include "ArduinoNATS.h"

#define NATS_CLIENT_ID_PREFIX "_CLIENTID"
#define NATS_CLIENT_ID_LENGTH 22

#define NATS_GUID_PREFIX "_GUID"
#define NATS_GUID_LENGTH 22


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

  char * clientID;

  static ConnectResponse connRes;

public:

  static bool connected;

// Constructor ---------------------------------------------//

public:
    StreamingNATS(Client* client, const char* hostname,
        int port = NATS_DEFAULT_PORT) :
      basicNats(new NATS(client, hostname, port)){}

// Methods ----------------------------------------------//

private:

  BYTE* buildMessage(Message msg, ...) {
    va_list args;
    va_start(args, msg);

    switch(msg) {
      case Msg_ConnectRequest:
        {
          ConnectRequest m = ConnectRequest_init_zero;

          strcpy(m.clientID, va_arg(args, char*));
          strcpy(m.heartbeatInbox, va_arg(args, char*));
          va_end(args);
  
          BYTE* buff = (BYTE*)calloc(1,ConnectRequest_size);
          pb_ostream_t stream = pb_ostream_from_buffer(buff, ConnectRequest_size);
          pb_encode(&stream, ConnectRequest_fields, &m);
          return buff;
        }
      case Msg_PubMsg:
        {
          PubMsg m = PubMsg_init_zero;
  
          strcpy(m.clientID, va_arg(args, char*));
          strcpy(m.guid, va_arg(args, char*));
          strcpy(m.subject, va_arg(args, char*));
          
          char* reply = va_arg(args, char*);
          if(reply) strcpy(m.reply, reply);
          
          char* data = va_arg(args, char*);
          m.data.size = strlen(data);
          Serial.println(m.data.size);
          if(data) strcpy((char*)m.data.bytes, data);
          
          char* sha256 = va_arg(args, char*);
          m.sha256.size = 0;
          if(sha256) strcpy((char*)m.sha256.bytes, sha256);
          va_end(args);
  
          BYTE* buff = (BYTE*)calloc(1, PubMsg_size);
          pb_ostream_t stream = pb_ostream_from_buffer(buff, PubMsg_size);
          pb_encode(&stream, PubMsg_fields, &m);
          return buff;
        }
    }
  }

  static void storeRes(NATS::msg msg) {
    ConnectResponse m = ConnectResponse_init_zero;
    pb_istream_t stream = pb_istream_from_buffer((unsigned char *)msg.data, msg.size);
    pb_decode(&stream, ConnectResponse_fields, &m);
    connRes = m;
    connected = true;
  }

  void processAll() {
    delay(1000);
    while(basicNats->client->available()) basicNats->process();
  }

  char* generate_clientId() {
    size_t size = (sizeof(NATS_CLIENT_ID_PREFIX) + NATS_CLIENT_ID_LENGTH) * sizeof(char);
    char* buf = (char*)malloc(size);
    strcpy(buf, NATS_CLIENT_ID_PREFIX);
    int i;
    for (i = sizeof(NATS_CLIENT_ID_PREFIX)-1; i < size-1; i++) {
      int random_idx = random(sizeof(NATSUtil::alphanums) - 1);
      buf[i] = NATSUtil::alphanums[random_idx];
    }
    buf[i] = '\0';
    return buf;
  }

  char* generate_guid() {
    size_t size = (sizeof(NATS_GUID_PREFIX) + NATS_GUID_LENGTH) * sizeof(char);
    char* buf = (char*)malloc(size);
    strcpy(buf, NATS_GUID_PREFIX);
    int i;
    for (i = sizeof(NATS_GUID_PREFIX)-1; i < size-1; i++) {
      int random_idx = random(sizeof(NATSUtil::alphanums) - 1);
      buf[i] = NATSUtil::alphanums[random_idx];
    }
    buf[i] = '\0';
    return buf;
  }

  char* generate_subject(char* subj) {
    size_t size = (strlen(connRes.pubPrefix) + strlen(subj) + 2) * sizeof(char);
    char* buf = (char*)malloc(size);
    strcpy(buf, connRes.pubPrefix);
    buf[strlen(connRes.pubPrefix)] = '.';
    strcpy(&buf[strlen(connRes.pubPrefix)+1], subj);
    buf[size-1] = '\0';
    return buf;
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
    clientID = generate_clientId();
    BYTE* buff= buildMessage(Msg_ConnectRequest, clientID, "heartbeatInbox");
//    basicNats->subscribe("_inbox.1", storeRes);
//    basicNats->publish("_STAN.discover.test-cluster", (char*)buff, "_inbox.1");

    basicNats->request("_STAN.discover.test-cluster", (char*)buff, storeRes);
    free(buff);
    processAll();
    return true;
  }

  void publish(char* subject, const char* msg = NULL, const char* replyto = NULL) {
    if (subject == NULL || subject[0] == 0) return;
    if (!connected) return;

    char* guid = generate_guid();
    BYTE* buff = buildMessage(Msg_PubMsg, clientID, guid, subject, NULL, msg, NULL); 
    char* pubSubj = generate_subject(subject);
    basicNats->publish(pubSubj, (char*)buff, replyto);
    free(pubSubj);
    free(guid);  
    free(buff);
  }

  void disconnect() {
    free(clientID);
  }
};


#endif
