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
    const auto Jobs = shell.GetCurrentJobs();
    for (size_t idx = 0; idx < Jobs.size(); ++idx)
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

bool disown(Shell &shell, const std::vector<std::string> &args)
{
    auto& Jobs = shell.GetCurrentJobs();
    if (Jobs.empty())
    {
        std::cout << "No jobs available\n";
        return true;
    }

    size_t idx = Jobs.size() - 1; // choose last one by default in case there was no second argument ( no idx passed )
    if (args.size() > 1)
    {
        idx = shell.ParseJobIndex(args);
        if (idx == -1) // parsing failed
            return false;
    }

    if(Jobs[idx].GetStatus() == JobStatus::STATUS_STOPPED)
    {
        printf("%s: warning: deleting stopped job %d with pid %d\n",shell.GetName().c_str(),idx,Jobs[idx].GetPID());
    }
    shell.RemoveJob(idx);
    return true;
}


} // namespace CMD
