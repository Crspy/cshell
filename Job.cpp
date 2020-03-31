#include "Job.hpp"


const char *GetStatusString(JobStatus status) {
    switch (status) {
        case JobStatus::STATUS_RUNNING:
            return "Running";
        case JobStatus::STATUS_DONE:
            return "Done";
        case JobStatus::STATUS_SUSPENDED:
            return "Suspended";
        case JobStatus::STATUS_TERMINATED:
            return "Terminated";
        default:
            return "Undefined";
    }
}
