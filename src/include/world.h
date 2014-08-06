#pragma once

#include "global.h"

using namespace rapidjson;

static const int SUPPORTED_WORLD_FILE_VERSION = 1;

class WorldFile {
    private:
        FILE *fp;
    public:
        std::string directory;
        std::string name;
        int version;

        WorldFile(std::string dir);

        bool open();
        bool close();

        ~WorldFile();
};

class World {

};

class Region {

};

class Block {

};