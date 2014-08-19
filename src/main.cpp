#include "main.h"

Server *s;

void catch_signal(int signum) {
    LOG.L("Recieved signal %d", signum);
    s->active = false;
    s->main_thread.join();
    delete(s);
    exit(signum);
}

void bind_signals() {
    signal(SIGINT, catch_signal);
}

int main(int argc, const char* argv[]) {
    bind_signals();

    // Load the SQLite3 Module stuff
    init_db_module();

    s = new Server("testworld", "Test Server");
    s->serve_forever();

    while (s->active) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}