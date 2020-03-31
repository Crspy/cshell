#include <iostream>
#include <cstdio>
#include "Shell.hpp"

int main(int argc, char *argv[])
{
    Shell sh;
    sh.Run(); // keeps running in a loop till it receives an exit command
    printf("lmao");
    return EXIT_SUCCESS;
}
