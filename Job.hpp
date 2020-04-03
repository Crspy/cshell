#pragma once
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <string>

enum class JobStatus
{
    STATUS_RUNNING,
    STATUS_EXITED,
    STATUS_STOPPED,
    STATUS_TERMINATED,
};
enum class ExecutionType
{
    BACKGROUND,
    FOREGROUND
};

class Job
{
    std::string m_Name;
    JobStatus m_Status;
    ExecutionType m_ExecType;
    pid_t m_Pid;

public:
    Job(const std::string &name, JobStatus status, ExecutionType execType, pid_t pid)
        : m_Name(name), m_Status(status), m_ExecType(execType), m_Pid(pid) {}

    void SetExecType(ExecutionType execType)
    {
        m_ExecType = execType;
    }

    ExecutionType GetExecType() const
    {
        return m_ExecType;
    }

    void SetStatus(JobStatus status)
    {
        m_Status = status;
    }

    JobStatus GetStatus() const
    {
        return m_Status;
    }

    pid_t GetPID() const
    {
        return m_Pid;
    }

    const std::string &GetName() const
    {
        return m_Name;
    }

    const char *GetStatusString() const
    {
        switch (m_Status)
        {
        case JobStatus::STATUS_RUNNING:
            return "Running";
        case JobStatus::STATUS_EXITED:
            return "Exited";
        case JobStatus::STATUS_STOPPED:
            return "Stopped";
        case JobStatus::STATUS_TERMINATED:
            return "Terminated";
        default:
            return "Undefined";
        }
    }
};
