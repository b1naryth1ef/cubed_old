#pragma once

#include "global.h"
#include "util/util.h"

/*
Cvars are client/server variables which take their inspiration from quake3.
They are meant to store data for either short-term or long-term durations.
This file contains a defintion for individual cvar handles, a global cvar
dictionary, and utilities to dump/load them from JSON files.

Example Usage:

CVarDict dict;

dict.load("user.json");
static CVar *sv_cheats = dict.get("sv_cheats");

sv_cheats.getInt();

TODO: rethink all this bullshit :(

*/

enum CVarFlag {
    FLAG_USER_READ = 1 << 0,
    FLAG_USER_WRITE = 1 << 1,
    FLAG_MOD_READ = 1 << 2,
    FLAG_MOD_WRITE = 1 << 3,
    FLAG_RUNTIME = 1 << 4,
    FLAG_CHEAT = 1 << 5,
};

static int FLAG_DEFAULT = (FLAG_USER_READ | FLAG_USER_WRITE | FLAG_MOD_READ | FLAG_MOD_WRITE);

class CVar: public Container {
    public:
        std::string name;
        std::string desc;
        int flags;

        CVar() {
            this->flags = FLAG_DEFAULT;
        }

        CVar *addFlag(CVarFlag f) {
            this->flags |= f;
            return this;
        }

        CVar *rmvFlag(CVarFlag f) {
            this->flags &= ~f;
            return this;
        }

        bool hasFlag(CVarFlag f) {
            return (this->flags & f);
        }

        CVar *setDesc(std::string d) {
            this->desc = d;
            return this;
        }

        Container *toContainer() {
            Container *c = new Container();
            switch (this->type) {
                case STORAGE_INT:
                    c->set(this->getInt());
                    break;
                case STORAGE_DOUBLE:
                    c->set(this->getDouble());
                    break;
                case STORAGE_STRING:
                    c->set(this->getString());
                    break;
            }
            return c;
        }

        void fromContainer(Container *c) {
            switch (c->type) {
                case STORAGE_INT:
                    this->set(c->getInt());
                    break;
                case STORAGE_DOUBLE:
                    this->set(c->getDouble());
                    break;
                case STORAGE_STRING:
                    this->set(c->getString());
                    break;
            }
        }
};

typedef std::function<bool (CVar *, Container *)> CVarOnChange;

class CVarDict {
    public:
        std::map<std::string, CVar*> data;
        std::vector<CVarOnChange> bindings;

        bool change(std::string k, Container *newc) {
            CVar *cvar = this->get(k);

            for (auto &f : this->bindings) {
                if (!f(cvar, newc)) {
                    break;
                }
            }

            cvar->fromContainer(newc);
            return true;
        }

        void bind(CVarOnChange f) {
            this->bindings.push_back(f);
        }

        CVar *get(std::string k) {
            if (!this->data.count(k)) {
                return this->create(k, "");
            }
            return data[k];
        }

        void set(std::string k, CVar *v) {
            data[k] = v;
        }

        CVar *create(std::string k, std::string d) {
            CVar *v = new CVar();
            v->name = k;
            v->desc = d;
            data[k] = v;
            return v;
        }

        bool load(std::string path) {
            return true;
        }
};
