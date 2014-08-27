#pragma once

#include "global.h"

class DB {
    public:
        sqlite3 *db;
        std::map<std::string, std::string> tables;
        std::map<std::string, sqlite3_stmt*> cached;
        bool is_new;

        DB(std::string path);
        ~DB();

        bool create();
        void addTable(std::string, std::string, bool);
        bool dropAll();
        bool drop(std::string table);

        bool addCached(std::string, std::string);
        sqlite3_stmt *getCached(std::string);
        void begin();
        void end();
};

void init_db_module();