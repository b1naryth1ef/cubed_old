#pragma once

#include "global.h"

#include <lua.hpp>


class Mod {
    public:
        std::string path;
        std::string name, author;
        int version;

        // Each mod has its own state, possibly more later?
        lua_State *L;

        Mod() {
            this->L = luaL_newstate();
            luaL_openlibs(L);
        }

        ~Mod() {
            lua_close(L);
        }

        void compile();
};

class ModDex {
    public:
        std::map<std::string, Mod*> mods;

        bool loadFromPath(std::string);

        Mod *get(std::string);
};

class Interface {};