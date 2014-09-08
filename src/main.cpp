#include "main.h"

void catch_signal(int signum) {
    DEBUG("Recieved signal %d, attempting exit", signum);

    if (s != nullptr) {
        s->active = false;
        s->main_thread.join();
        delete(s);
    }

    if (c != nullptr) {
        c->active = false;
        delete(c);
    }

    exit(signum);
}

void bind_signals() {
    signal(SIGINT, catch_signal);
}

void run_server() {
    bind_signals();

    s = new Server();
    s->serve_forever();

    while (s->active || c->active) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void run_client() {
    bind_signals();

    c = new Client();
    c->setup();
    c->run();

    while (c->active) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

int main(int argc, const char* argv[]) {
    if (argc > 1) {
        if (strcmp(argv[1], "client") == 0) {
            run_client();
            return 0;
        } else if (strcmp(argv[1], "server") == 0) {
            run_server();
            return 0;
        }
    }

    printf("Usage: ./cubed.o <client|server>\n");
    return 1;
}