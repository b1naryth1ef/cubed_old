#pragma once

#include "global.h"
#include "util/util.h"

class DB;

class DBQuery {
    private:
        DB* db;
        sqlite3_stmt* stmt;
        std::mutex mutex;
        uint16_t index;
        int result;

    public:
        DBQuery(DB*, sqlite3_stmt*);

        void reset();
        void end();

        // Execution and checks
        int execute();

        bool success() {
            return (this->result != SQLITE_ERROR);
        }

        bool empty() {
            return (this->result == SQLITE_DONE);
        }

        int getLastInsertID();

        // Binding data
        void bindText(std::string);
        void bindInt(int);

        // Grabbing data
        int getInt(int);
        std::string getText(int);
};

class DB {
    private:
        std::map<std::string, DBQuery*> cached;

    public:
        sqlite3 *db;
        std::map<std::string, std::string> tables;
        bool is_new;

        DB(std::string path);
        ~DB();

        bool create();
        void addTable(std::string, std::string, bool);
        bool dropAll();
        bool drop(std::string table);

        DBQuery* addCached(std::string);
        void begin();
        void end();

        static void init_sqlite();

        DBQuery* query(std::string);
};

