#pragma once

#include "global.h"
#include "geo.h"

class Entity {
    public:
        Point *pos;
        short health;

        // The number of blocks around this entity to keep loaded, 0 is none
        virtual int keepWorldLoadedAround() { return 0; };

        // Whether this entity persists in the world-database
        virtual bool doesPersist() { return false; };
};

class Player {
    int keepWorldLoadedAround() {
        return 64;
    }

    bool doesPersist() {
        return false;
    }
};