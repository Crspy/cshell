#include "CMD.hpp"
#include "Shell.hpp"

namespace CMD
{
void cd(Shell &shell, const std::vector<std::string> &args)
{
    std::string path;
    if (args.size() == 1)
    {
        // goto home path if cd has no args
        path = std::getenv("HOME");
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
    shell.SetPrevWorkingDir(fs::current_path()); // save current path as old working directory
    if (chdir(path.c_str()) != 0)
    {
        perror(shell.GetName());
    }
}

void jobs(Shell &shell, const std::vector<std::string> &args)
{
    shell.UpdateJobsStatus();
    const auto count = shell.GetCurrentJobs().size();
    for (size_t idx = 0; idx < count; ++idx)
    {
        shell.PrintJobStatus(idx);
    }
}

bool fg(Shell &shell, const std::vector<std::string> &args)
{
    size_t idx = 0; // choose first one by default in case there was no second argument
    if (args.size() > 1)
    {
        if (args[1].find_first_of('%') == std::string::npos)
            return false;                                    // if % can't be found then it's a syntax error
        const auto &idx_it = args[1].find_first_not_of('%'); // the index should come after % directly
        if (idx_it == std::string::npos)
            return false;                             // if nothing found after % then it's an error
        std::string idx_str = args[1].substr(idx_it); // extract the string that contains the index
        bool bHasDigitsOnly = (idx_str.find_first_not_of("0123456789") == std::string::npos);
        if (bHasDigitsOnly)
        {
            std::stringstream ss(idx_str);
            ss >> idx; // convert the string to an int (parse as an int)
            if (idx > (shell.GetCurrentJobs().size() - 1))
                return false;
        }
        else
            return false;
    }
    if (shell.GetCurrentJobs().empty())
    {
        std::cout << "No jobs available\n";
    }
    else
    {
        // send CONTINUE signal in case the process was stopped
        if (kill(shell.GetCurrentJobs()[idx].m_Pid, SIGCONT) < 0)
        {
            return false;
        }
        shell.WaitForJob(idx); // put first job in background
    }

    return true;
}
} // namespace CMD
