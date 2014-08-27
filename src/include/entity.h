#pragma once

#include "global.h"
#include "geo.h"

class Entity {
    public:
        Point *pos;

        // The number of blocks around this entity to keep loaded, 0 is none
        virtual int keepWorldLoadedAround() { return 0; };

        // Whether this entity persists in the world-database
        virtual bool doesPersist() { return false; };

        virtual bool doesTakeDamage() { return false; };
};

class LivingEntity: public Entity {
    public:
        short health;
        long int age;

        virtual bool isAlive() { return health > 0; }

        virtual bool doesTakeDamage() { return true; }
};

class Player {
    public:
        std::string name;


        int keepWorldLoadedAround() {
            return 64;
        }

        bool doesPersist() {
            return false;
        }
};