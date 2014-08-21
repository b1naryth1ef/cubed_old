#pragma once

#include "global.h"
#include "util.h"

enum EventState {
    STATE_PRE,
    STATE_POST
};

class Event {
    public:
        EventState state;
        Dict *data;

        bool cancelled;
};