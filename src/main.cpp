#include "main.h"

int main(int argc, const char* argv[]) {
    // Load the SQLite3 Module stuff
    init_db_module();

    Server s("testworld", "Test Server");
    // s.server_forever();
}