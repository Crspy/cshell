#pragma once

#include <vector>
#include <string>

class Shell; // forward declaration

namespace CMD {
    void cd(Shell &shell, const std::vector<std::string> &args);

    bool fg(Shell &shell, const std::vector<std::string> &args);

    void jobs(Shell &shell, const std::vector<std::string> &args);
}
