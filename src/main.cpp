#include "main.h"

int main(int argc, const char* argv[]) {
    LOG.L("This is a test!");
    WorldFile w = WorldFile("testworld");
    w.open();
}