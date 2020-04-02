#pragma once
#include <iostream>
#include <algorithm>
#include <iterator>
#include <cstring>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <memory>
#include <cstdio>
#include <unistd.h>
#include <wait.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "Util.hpp"
#include "Job.hpp"
#include "CMD.hpp"

class Shell
{
    std::ofstream m_LogFile;
    const std::string m_ShellName = "cshell";
    std::string m_PrevWorkingDir; // previous working directory
    std::string m_CurrUsername;     // current logged in user name
    std::vector<Job> m_CurrentJobs; // current jobs launched by shell

    
    std::string ReadLine();

    // returns bool if args[0] isn't a builtin command
    bool ExecuteBuiltinCommands(const std::vector<std::string> &args);

    void Parse(std::vector<std::string> &args);

    void LaunchJob(std::vector<std::string> &args);



    void PrintPrompt()
    {
        #define COLOR_BOLD_GREEN "\033[1;32m"
        #define COLOR_NONE "\033[0m"

        printf(COLOR_BOLD_GREEN "%s" COLOR_NONE " >> ",
               m_CurrUsername.c_str());
    }

public:
    Shell();

    void Run();

    void WaitForJob(int idx);

    // helper function for  'bg' and 'fg' commands.
    // returns false if Job idx specified doesn't exist or there was a syntax error.Otherwise , returns true.
    // bSendToForeground : if true the job is continued in foreground. if false it's continued in background
    bool ContinueJob(const std::vector<std::string> &args, bool bSendToForeground);

    std::string GetName()
    {
        return m_ShellName;
    }

    // returns the absolute path in which the cshell binary exists
    std::string GetAbsolutePath();

    std::string GetCurrWorkingDir();

    void SetPrevWorkingDir(const std::string &Dir)
    {
        m_PrevWorkingDir = std::move(Dir);
    }

    const std::string &GetPrevWorkingDir()
    {
        return m_PrevWorkingDir;
    }

    std::vector<Job> &GetCurrentJobs()
    {
        return m_CurrentJobs;
    }

    void UpdateJobsStatus();

    void RemoveJob(int id)
    {
        m_CurrentJobs.erase(m_CurrentJobs.begin() + id);
    }

    int GetJobIdxByPID(pid_t pid)
    {
        const auto count = m_CurrentJobs.size();
        for (size_t idx = 0; idx < count; ++idx)
        {
            if (m_CurrentJobs[idx].GetPID() == pid)
            {
                return idx;
            }
        }
        return -1; // not found
    }

    void PrintJobStatus(int idx, bool bPrintStatus = true)
    {
        const auto &job = m_CurrentJobs[idx];
        if (bPrintStatus)
            printf("[%d]\t%d\t%s\t\t%s\n", idx, job.GetPID(), job.GetStatusString(), job.GetName().c_str());
        else
        {
            printf("[%d]\t%d\n", idx, job.GetPID());
        }
    }
};

extern Shell gShell;
