#include "CMD.hpp"
#include "Shell.hpp"

namespace CMD
{
void cd(Shell &shell, const std::vector<std::string> &args)
{
    std::string path;
    if (args.size() == 1)
    {
        return; // do nothing
    }
    else
    {
        // now we are sure args[1] exist
        path = args[1];
        if (args[1] == "-")
        {
            path = shell.GetPrevWorkingDir(); // go to previous working directory
        }
    }
    shell.SetPrevWorkingDir(shell.GetCurrWorkingDir()); // save current path as old working directory
    if (chdir(path.c_str()) != 0)
    {
        perror(shell.GetName().c_str());
    }
}

void jobs(Shell &shell, const std::vector<std::string> &args)
{
    const auto count = shell.GetCurrentJobs().size();
    for (size_t idx = 0; idx < count; ++idx)
    {
        shell.PrintJobStatus(idx);
    }
}

bool bg(Shell &shell, const std::vector<std::string> &args)
{
    return shell.ContinueJob(args, false);
}

bool fg(Shell &shell, const std::vector<std::string> &args)
{
    return shell.ContinueJob(args, true);
}

} // namespace CMD
