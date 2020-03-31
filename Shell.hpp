#pragma once
#include <iostream>
#include <string_view>
#include <filesystem>
#include <cstdio>
#include <algorithm>
#include <iterator>
#include <cstring>
#include <string>
#include <vector>
#include <memory>
#include <unistd.h>
#include <wait.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "Util.hpp"
#include "Job.hpp"
#include "CMD.hpp"

namespace fs = std::filesystem;

class Shell
{
    const char *const m_ShellName = "cshell";
    std::string m_PrevWorkingDir;    // previous working directory
    std::string_view m_CurrUsername; // current user name (immutable string)
    std::vector<Job> m_CurrentJobs;  // current jobs launched by shell


    std::string ReadLine();

    void Parse(std::vector<std::string> &args);

    

    void LaunchJob(std::vector<std::string> &args);

    void PrintPrompt()
    {
        std::cout << m_CurrUsername << " : " << fs::current_path();
        std::cout << " >> ";
    }

public:
    Shell();
    
    void Run();

    void WaitForJob(int idx);
    
    auto GetName()
    {
        return m_ShellName;
    }

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
            if (m_CurrentJobs[idx].m_Pid == pid)
            {
                return idx;
            }
        }
        throw std::runtime_error("invalid pid passed to GetJobIDByPID");
    }

    void PrintJobStatus(int idx, bool bPrintStatus = true)
    {
        const auto &job = m_CurrentJobs[idx];
        if (bPrintStatus)
            printf("[%d]\t%d\t%s\t%s\n", idx, job.m_Pid, GetStatusString(job.m_Status), job.m_Name.c_str());
        else
        {
            printf("[%d]\t%d\t%s\n", idx, job.m_Pid, job.m_Name.c_str());
        }
    }

};
