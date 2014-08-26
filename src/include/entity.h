#pragma once

#include "global.h"
#include "geo.h"

class Entity {
    public:
        Point *pos;

        // The number of blocks around this entity to keep loaded, 0 is none
        int keepWorldLoadedAround() { return 0; };

        // Whether this entity persists in the world-database
        bool doesPersist() { return false; };
};

class Player {
    virtual int keepWorldLoadedAround() {
        return 64;
    }

    virtual bool doesPersist() {
        return false;
    }
};