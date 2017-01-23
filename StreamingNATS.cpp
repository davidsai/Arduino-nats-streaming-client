#include "StreamingNATS.h"

ConnectResponse StreamingNATS::connRes;
bool StreamingNATS::connected = false;
NATS* StreamingNATS::basicNats;

