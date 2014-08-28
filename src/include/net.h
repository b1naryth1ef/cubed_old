#pragma once

#include "global.h"

#include "flatbuffers/flatbuffers.h"

// generated Files
#include "gen/packet_generated.h"
#include "gen/handshake_generated.h"

class Packet {};

class NetServer {
    public:
        NetServer(std::string host, short port) {}
};
