#pragma once

#include "global.h"
#include "util/util.h"

enum EventState {
    STATE_NULL,
    STATE_PRE,
    STATE_POST
};


class Event {
    public:
        std::string type;
        EventState state;
        Dict *data;

        bool cancelled;

        Event(std::string s) {
            this->type = s;
            this->data = new Dict();
            this->state = STATE_NULL;
            this->cancelled = false;
        }
};
