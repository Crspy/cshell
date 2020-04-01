#pragma once

#include <sys/types.h>
#include <string>

enum class JobStatus
{
    STATUS_RUNNING,
    STATUS_DONE,
    STATUS_STOPPED,
    STATUS_TERMINATED,
};

class Job
{
    std::string m_Name;
    JobStatus m_Status;
    pid_t m_Pid;
public:

    Job(const std::string &name, JobStatus status, pid_t pid)
        : m_Name(name), m_Status(status), m_Pid(pid)
    {

    }

    void SetStatus(JobStatus status)
    {
        m_Status = status;
    }

    pid_t GetPID() const
    {
        return m_Pid;
    }

    const std::string& GetName() const
    {
        return m_Name;
    }

    const char *GetStatusString() const
    {
        switch (m_Status)
        {
        case JobStatus::STATUS_RUNNING:
            return "Running";
        case JobStatus::STATUS_DONE:
            return "Done";
        case JobStatus::STATUS_STOPPED:
            return "Stopped";
        case JobStatus::STATUS_TERMINATED:
            return "Terminated";
        default:
            return "Undefined";
        }
    }
};
