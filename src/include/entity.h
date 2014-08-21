#pragma once

#include "global.h"

class Entity {
    public:
        // Whether this entity forces things around it to be loaded
        bool does_force_load_around();

        // Whether this entity persists in the database
        bool does_persist();
};

class Player {
    virtual bool does_force_load_around() {
        return true;
    }

    virtual bool does_persist() {
        return false;
    }
};