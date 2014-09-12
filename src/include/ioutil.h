/*
This holds utilities for disk and network IO
*/

#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <vector>
#include <pwd.h>

namespace ioutil {

typedef std::vector<std::string> vstring;

// Returns true if the file at path file_name exists
inline bool file_exists(std::string file_name) {
    struct stat buffer;
    return (stat (file_name.c_str(), &buffer) == 0);
}

inline std::string join(std::string a, std::string b) {
    return a + "/" + b;
}

static std::string getDataDirectory() {
    struct passwd *pw = getpwuid(getuid());
    std::string res = join(pw->pw_dir, ".cubed");
    return res;
}

static std::string setupDataDirectory() {
    std::string dir = getDataDirectory();
    if (!file_exists(dir)) {
        mkdir(dir.c_str(), 0777);
    }
    return dir;
}

// Multiwriter is a simple class that allows writing to multiple ostream objects at once.
class MultiWriter {
    private:
        std::vector<std::ostream*> mwli;

    public:
        MultiWriter() {}

        // Add an ostream pointer to the list of printables
        void add(std::ostream *f) {
            mwli.push_back(f);
        }

        // Write to the ostreams (all at once)
        void write(const char* s, std::streamsize n) {
            for ( auto &i : mwli ) {
                i->write(s, n);
            }
        }
};

}