#pragma once

#include "global.h"
#include "util/util.h"

namespace Net {

/*
    Channels provide a way for the server and mods to transmit lower-priority or bulk data over side-channels.
    They also enable P2P communication.
*/
template <class T> class Channel {
    private:
        T* client;

        void onTCPEvent(TCPEvent&);
}

}
