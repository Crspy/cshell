#pragma once

#include <vector>
#include <string>

class Shell; // forward declaration

namespace CMD
{
void cd(Shell &shell, const std::vector<std::string> &args);
void jobs(Shell &shell, const std::vector<std::string> &args);

/*
    the following functions:
    returns false if Job idx specified doesn't exist or there was a syntax error.
    Otherwise , returns true.
*/
bool bg(Shell &shell, const std::vector<std::string> &args);
bool fg(Shell &shell, const std::vector<std::string> &args);
bool disown(Shell &shell, const std::vector<std::string> &args);

} // namespace CMD
