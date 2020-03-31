#include "Shell.hpp"

Shell::Shell() : m_PrevWorkingDir(fs::current_path())
{
    /*
     replace behaviour of (CTRL + C) which terminates the shell
     instead just print newline just like how bash does it
    */
    struct sigaction sigint_action;
    sigint_action.sa_handler = [](int signal) {
        std::cout << std::endl;
    };
    sigint_action.sa_flags = 0;
    sigemptyset(&sigint_action.sa_mask);
    sigaction(SIGINT, &sigint_action, NULL);

    // prevent dump core signal
    signal(SIGQUIT, SIG_IGN);

    // prevent jobs running in background from stopping the shell
    // on read/write operations to the terminal
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);

    pid_t pid = getpid();         // get pid of the shell
    setpgid(pid, pid);            // set process group id of the shell process to be itself
    tcsetpgrp(STDIN_FILENO, pid); // make shell pgid the foreground pgid on the termianl associated to STDIN_FILENO

    // get our current logged-in username
    struct passwd *pwd = getpwuid(getuid());
    if (pwd)
        m_CurrUsername = pwd->pw_name;
    else
        m_CurrUsername = "(?)";
}

void Shell::Run()
{
    while (true)
    {
        UpdateJobsStatus();

        PrintPrompt();

        std::string line = ReadLine();
        if (!line.empty())
        {
            std::vector<std::string> args = Util::Tokenize(line, " \t\n");
            Parse(args);
        }
    }
}

std::string Shell::ReadLine()
{
    std::string line;
    std::getline(std::cin, line);
    if (std::cin.fail() || std::cin.eof())
    {
        // reset cin state in case of some signal that caused the input stream to close
        std::cin.clear();
    }
    return line;
}

void Shell::UpdateJobsStatus()
{
    int status, pid;
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0)
    {
        const int idx = GetJobIdxByPID(pid);
        if (WIFEXITED(status))
        {
            m_CurrentJobs[idx].m_Status = JobStatus::STATUS_DONE;
            PrintJobStatus(idx);
            RemoveJob(idx);
        }
        else if (WIFSIGNALED(status))
        {
            m_CurrentJobs[idx].m_Status = JobStatus::STATUS_TERMINATED;
            PrintJobStatus(idx);
            RemoveJob(idx);
        }
        else if (WIFSTOPPED(status))
        {
            m_CurrentJobs[idx].m_Status = JobStatus::STATUS_SUSPENDED;
        }
        else if (WIFCONTINUED(status))
        {
            m_CurrentJobs[idx].m_Status = JobStatus::STATUS_RUNNING;
        }
    }
}

void Shell::Parse(std::vector<std::string> &args)
{
    if (args.empty())
        return;

    if (args[0] == "cd")
    {
        CMD::cd(*this, args);
    }
    else if (args[0] == "jobs")
    {
        CMD::jobs(*this, args);
    }
    else if (args[0] == "fg")
    {
        if (!CMD::fg(*this, args))
        {
            std::cout << GetName() << ": fg: " << args[1] << ": no such job\n";
        }
    }
    else if (args[0] == "exit")
    {
        exit(EXIT_SUCCESS);
    }
    else
    {
        LaunchJob(args);
    }
}

void Shell::WaitForJob(int idx)
{
    const int pid = m_CurrentJobs[idx].m_Pid;

    // let fgPID control the terminal fd
    tcsetpgrp(STDIN_FILENO, pid);


    int status = 0;
    int wait_pid = waitpid(pid, &status, WUNTRACED);
    if (wait_pid == pid)
    {
        if (WIFEXITED(status))
        {
            m_CurrentJobs[idx].m_Status = JobStatus::STATUS_DONE;
            RemoveJob(idx);
        }
        else if (WIFSIGNALED(status))
        {
            m_CurrentJobs[idx].m_Status = JobStatus::STATUS_TERMINATED;
            RemoveJob(idx);
        }
        else if (WIFSTOPPED(status))
        {
            m_CurrentJobs[idx].m_Status = JobStatus::STATUS_SUSPENDED;
        }
    }

    // ignore SIGTTOU signal temporarily to prevent shell from getting stopped by foreground process when it exits/stops/terminates
    signal(SIGTTOU, SIG_IGN);
    tcsetpgrp(STDIN_FILENO, getpid()); // restore terminal control to shell
    signal(SIGTTOU, SIG_DFL); // stop ignoring SIGTTOU signal since our shell is back in control
}

void Shell::LaunchJob(std::vector<std::string> &args)
{
    bool bIsBackgroundExec = false;
    auto &lastArg = args[args.size() - 1];
    if (lastArg[lastArg.size() - 1] == '&')
    {
        bIsBackgroundExec = true;

        // check if & is a whole arg on its own
        // and if & isn' the only arg
        if (lastArg == "&" && args.size() > 1)
        {
            args.pop_back();
        }
        else
        {
            // otherwise & is just attached to the last arg like "firefox&
            // so we just remove it by replacing it with a null terminator
            lastArg[lastArg.size() - 1] = '\0';
        }
    }

    // parse %job_idx
    for (size_t i = 1; i < args.size(); ++i)
    {
        if (args[i].find_first_of('%') == std::string::npos)
            continue;
        const auto &idx_it = args[i].find_first_not_of('%'); // the index should come after % directly
        if (idx_it == std::string::npos)
            continue; // if nothing found after % then it's not an index

        std::string idx_str = args[i].substr(idx_it); // extract the string that contains the index
        bool bHasDigitsOnly = (idx_str.find_first_not_of("0123456789") == std::string::npos);
        if (bHasDigitsOnly)
        {
            int idx = 0;
            std::stringstream ss(std::move(idx_str));
            ss >> idx; // convert the string to an int (parse as an int)
            if (idx <= (m_CurrentJobs.size() - 1))
                args[i] = std::to_string(m_CurrentJobs[idx].m_Pid);
        }
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        // in child process

        // restore default signals behaviour
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

        setpgid(0, 0);

        /*
         create an array to hold the args pointers from the vector
         which clean itself automatically in case execvp fails

         we need extra arg place at the end to contain nullptr so that execvp can deduce args count
        */
        auto cargs = std::make_unique<char *[]>(args.size() + 1);
        for (size_t idx = 0; idx < args.size(); ++idx)
        {
            cargs[idx] = &args[idx][0];
        }
        cargs[args.size()] = nullptr; // last element in argv array must be null

        if (execvp(cargs[0], cargs.get()) == -1)
        {
            perror(GetName());
            std::cout << "err in execvp " << args.size() << "\n";
        }

        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
        perror(GetName());
    }
    else
    {
        // in parent process

        m_CurrentJobs.emplace_back(args[0], JobStatus::STATUS_RUNNING, pid);
        if (bIsBackgroundExec)
        {
            PrintJobStatus(m_CurrentJobs.size() - 1, false); // print latest job without status
        }
        else
        {
            WaitForJob(m_CurrentJobs.size() -
                       1); // wait for foreground process (latest job) till it gets stopped or terminated
        }
    }
}
