#pragma once

#include <sys/types.h>
#include <string>


enum class JobStatus {
    STATUS_RUNNING,
    STATUS_DONE,
    STATUS_SUSPENDED,
    STATUS_TERMINATED,
};

const char *GetStatusString(JobStatus status);


struct Job {
    std::string m_Name;
    JobStatus m_Status;
    pid_t m_Pid;

    Job(std::string &name, JobStatus status, pid_t pid)
            : m_Name(name), m_Status(status), m_Pid(pid) {

    }
};
