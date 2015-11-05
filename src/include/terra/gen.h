#pragma once

#include "global.h"
#include "terra/terra.h"
#include "util/geo.h"

namespace Terra {

class World;

class WorldGenerator {
    public:
        std::string seed;
        World *world;

        virtual void generate_area(BoundingBox) = 0;
};

class FlatGenerator : public WorldGenerator {
    public:
        FlatGenerator(std::string seed, World* w) {
            this->seed = seed;
            this->world = w;
        }

        void generate_area(BoundingBox);
};

}
