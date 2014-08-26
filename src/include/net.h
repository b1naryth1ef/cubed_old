#pragma once

#include "global.h"

#include "flatbuffers/flatbuffers.h"

// generated Files
#include "gen/packet_generated.h"
#include "gen/handshake_generated.h"

class Channel {};
class RemoteClient {

};

class NetClient {};
class NetClientChannel: public Channel {};

class NetService {};
class NetServiceChannel: public Channel {};

class Packet {
    public:
        NetClient *cli;
        Channel *chan;
};
