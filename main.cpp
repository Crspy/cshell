#include <iostream>
#include "Shell.hpp"

int main(int argc, char *argv[])
{
    Shell sh;
    sh.Run(); // keeps running in a loop till it receives an exit command
    return EXIT_SUCCESS;
}
