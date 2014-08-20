#include "main.h"

Server *s;

void catch_signal(int signum) {
    DEBUG("Recieved signal %d, attempting exit", signum);
    s->active = false;
    s->main_thread.join();
    s->net_thread.join();
    delete(s);
    exit(signum);
}

void bind_signals() {
    signal(SIGINT, catch_signal);
}

int main(int argc, const char* argv[]) {
    bind_signals();

    s = new Server("testworld", "Test Server", 64);
    s->serve_forever();

    while (s->active) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}