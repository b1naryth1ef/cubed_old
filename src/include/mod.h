#pragma once

#include "global.h"
#include "events.h"
#include "db.h"
#include <lua.hpp>

class ModDex;

class Mod {
    public:
        std::string path;
        std::string name, author;
        int version;

        ModDex *dex;

        // Each mod has its own state, possibly more later?
        lua_State *L;

        Mod() {
            this->L = luaL_newstate();
            luaL_openlibs(L);
        }

        ~Mod() {
            lua_close(L);
        }

        void load();
};

class ModDex {
    public:
        DB *db;
        std::map<std::string, Mod*> mods;
        bool loadFromPath(std::string);

        Mod *get(std::string);

        bool eventPre(Event*);
        bool eventPost(Event*);
};

class Interface {};

static void loadEventsModule(ModDex *);