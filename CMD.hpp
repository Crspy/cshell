#pragma once

#include <vector>
#include <string>

class Shell; // forward declaration

namespace CMD {
    void cd(Shell &shell, const std::vector<std::string> &args);

    /*
        returns false if Job idx specified doesn't exist or there was a syntax error.
        Otherwise , returns true.
    */
    bool bg(Shell &shell, const std::vector<std::string> &args);

    /*
        returns false if Job idx specified doesn't exist or there was a syntax error.
        Otherwise , returns true.
    */
    bool fg(Shell &shell, const std::vector<std::string> &args);

    void jobs(Shell &shell, const std::vector<std::string> &args);
}
