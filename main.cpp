#include <iostream>
#include "Shell.hpp"

Shell gShell; // gShell is created singleton so that we can use it inside signal handlers

int main(int argc, char *argv[])
{
    gShell.Run(); // keeps running in a loop till it receives an exit command
    return EXIT_SUCCESS;
}
