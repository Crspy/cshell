#include "Shell.hpp"

Shell::Shell() : m_PrevWorkingDir(GetCurrWorkingDir())
{
    std::string absolutePath = GetAbsolutePath();
    if (!absolutePath.empty())
        m_LogFile = std::ofstream(absolutePath + ".log", std::ios::trunc);
    else
    {
        std::cout << "Failed to get absolute path of the shell.\n";
        std::cout << "Log file will be created in current working directory instead.\n";
        m_LogFile = std::ofstream(GetName() + ".log", std::ios::trunc);
    }

    // add a handler for SIGCHLD signal to update the children status and log them
    auto SIGCHLD_Handler = [](int signal) {
        gShell.UpdateJobsStatus();
    };

    signal(SIGCHLD, SIGCHLD_Handler);

    /*
        replace behaviour of (CTRL + C) which terminates the shell
        and  (CTRL + Z)  which stops the shell
        and  (CTRL + \)  which dumps core and terminates the shell
        to just print newline just like how most shell does it
    */
    struct sigaction sigIgnore_action;
    sigIgnore_action.sa_handler = [](int signal) {
        std::cout << std::endl;
    };
    sigIgnore_action.sa_flags = 0;
    sigemptyset(&sigIgnore_action.sa_mask);
    sigaction(SIGINT, &sigIgnore_action, NULL);
    sigaction(SIGQUIT, &sigIgnore_action, NULL);
    sigaction(SIGTSTP, &sigIgnore_action, NULL);

    // prevent jobs running in background from stopping the shell
    // when trying to read/write from terminal
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    pid_t pid = getpid(); // get pid of the shell
    setpgid(pid, pid);    // set process group id of the shell process to be itself

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
    std::cout << "Welcome to " << m_ShellName << '\n';

    while (true)
    {
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

    // we call waitpid with -1 to check for any child process that has changed status
    // we use WNOHANG to prevent waitpid from blocking (returns immediately)
    // we use WUNTRACED to get a report about status of stopped children
    // we use WCONTINUED to get a report about status continued children
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0)
    {
        const int idx = GetJobIdxByPID(pid);
        if (idx == -1)
            continue;

        if (WIFEXITED(status))
        {
            m_CurrentJobs[idx].SetStatus(JobStatus::STATUS_EXITED);
            m_LogFile << m_CurrentJobs[idx].GetName() << '\t' << m_CurrentJobs[idx].GetStatusString() << std::endl;
            RemoveJob(idx);
        }
        else if (WIFSIGNALED(status))
        {
            m_CurrentJobs[idx].SetStatus(JobStatus::STATUS_TERMINATED);
            m_LogFile << m_CurrentJobs[idx].GetName() << '\t' << m_CurrentJobs[idx].GetStatusString() << std::endl;
            RemoveJob(idx);
        }
        else if (WIFSTOPPED(status))
        {
            m_CurrentJobs[idx].SetStatus(JobStatus::STATUS_STOPPED);
            m_LogFile << m_CurrentJobs[idx].GetName() << '\t' << m_CurrentJobs[idx].GetStatusString() << std::endl;
        }
        else if (WIFCONTINUED(status))
        {
            m_CurrentJobs[idx].SetStatus(JobStatus::STATUS_RUNNING);
            m_LogFile << m_CurrentJobs[idx].GetName() << "\tContinued" << std::endl;
        }
    }
}

void Shell::Parse(std::vector<std::string> &args)
{
    if (args.empty())
        return; // do nothing

    if (args.size() > 1)
    {
        for (auto &arg : args)
        {
            // check for tilde and replace it with HOME path
            if (arg[0] == '~')
            {
                // we check what comes after '~'  is either '/' or  just nothing
                // just to make sure it's a path not a folder/file name
                if (arg[1] == '/' || arg.size() == 1)
                {
                    arg.replace(0, 1, getenv("HOME"));
                }
            }
        }
    }

    if (!ExecuteBuiltinCommands(args))
    {
        // if it's not a builtin command
        // then it must be an external program to be launched
        LaunchJob(args);
    }
}

bool Shell::ExecuteBuiltinCommands(const std::vector<std::string> &args)
{
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
    else if (args[0] == "bg")
    {
        if (!CMD::bg(*this, args))
        {
            std::cout << GetName() << ": bg: " << args[1] << ": no such job\n";
        }
    }
    else if (args[0] == "disown")
    {
        if (!CMD::disown(*this, args))
        {
            std::cout << GetName() << ": disown: " << args[1] << ": no such job\n";
        }
    }
    else if (args[0] == "exit")
    {
        m_LogFile.close();
        exit(EXIT_SUCCESS);
    }
    else
    {
        return false; // not a builtin command
    }

    return true;
}

void Shell::WaitForJob(int idx)
{
    const int pid = m_CurrentJobs[idx].GetPID();

    // let fgPID control the terminal fd
    tcsetpgrp(STDIN_FILENO, pid);

    int status = 0;
    // here we use waitpid to just keep polling status of the specified child pid
    // we use WUNTRACED to return if the child process was stopped
    int wait_pid = waitpid(pid, &status, WUNTRACED);
    if (wait_pid == pid)
    {
        if (WIFEXITED(status))
        {
            m_CurrentJobs[idx].SetStatus(JobStatus::STATUS_EXITED);
            m_LogFile << m_CurrentJobs[idx].GetName() << '\t' << m_CurrentJobs[idx].GetStatusString() << std::endl;
            RemoveJob(idx);
        }
        else if (WIFSIGNALED(status))
        {
            // in case we termianted by CTRL + C , a "^C" will be echoed
            // and we want to avoid printing our prompt after that
            // so we just print a newline
            putchar('\n');
            m_CurrentJobs[idx].SetStatus(JobStatus::STATUS_TERMINATED);
            m_LogFile << m_CurrentJobs[idx].GetName() << '\t' << m_CurrentJobs[idx].GetStatusString() << std::endl;
            RemoveJob(idx);
        }
        else if (WIFSTOPPED(status))
        {
            // in case we stopped the process by CTRL + Z , a "^Z" will be echoed
            // and we want to avoid printing our prompt after that
            // so we just print a newline
            putchar('\n');
            m_CurrentJobs[idx].SetStatus(JobStatus::STATUS_STOPPED);
            m_LogFile << m_CurrentJobs[idx].GetName() << '\t' << m_CurrentJobs[idx].GetStatusString() << std::endl;
        }
    }

    tcsetpgrp(STDIN_FILENO, getpid()); // restore terminal control to shell
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
            // so we just remove it
            lastArg.pop_back();
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
                args[i] = std::to_string(m_CurrentJobs[idx].GetPID());
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

        // set the child process group id to be its own pid
        // so that the child don't terminate when we exit shell
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
            perror(GetName().c_str());
        }

        exit(EXIT_FAILURE);
    }
    else if (pid == -1)
    {
        // failed to fork
        perror(GetName().c_str());
    }
    else
    {
        // in parent process

        if (bIsBackgroundExec)
        {
            m_CurrentJobs.emplace_back(Job(args[0], JobStatus::STATUS_RUNNING, ExecutionType::BACKGROUND, pid));
            // print latest job without status
            PrintJobStatus(m_CurrentJobs.size() - 1, false);
        }
        else
        {
            m_CurrentJobs.emplace_back(Job(args[0], JobStatus::STATUS_RUNNING, ExecutionType::FOREGROUND, pid));
            // wait for foreground process (latest job) till it gets stopped or terminated
            WaitForJob(m_CurrentJobs.size() - 1);
        }
    }
}

int Shell::ParseJobIndex(const std::vector<std::string> &args)
{
    int idx;
    if (args[1].find_first_of('%') == std::string::npos)
        return -1; // if % can't be found then it's a syntax error

    const auto &idx_it = args[1].find_first_not_of('%'); // the index should come after % directly
    if (idx_it == std::string::npos)
        return -1; // if nothing found after % then it's an error

    std::string idx_str = args[1].substr(idx_it); // extract the string that contains the index
    bool bHasDigitsOnly = (idx_str.find_first_not_of("0123456789") == std::string::npos);
    if (bHasDigitsOnly)
    {
        std::stringstream ss(idx_str);
        ss >> idx; // convert the string to an int (parse as an int)
        if (idx > (m_CurrentJobs.size() - 1))
            return -1;
    }
    else
        return -1;

    return idx;
}

bool Shell::ContinueJob(const std::vector<std::string> &args, bool bSendToForeground)
{
    if (m_CurrentJobs.empty())
    {
        std::cout << "No jobs available\n";
        return true;
    }

    size_t idx = m_CurrentJobs.size() - 1; // choose last one by default in case there was no second argument ( no idx passed )
    if (args.size() > 1)
    {
        idx = ParseJobIndex(args);
        if (idx == -1) // parsing failed
            return false;
    }

    // send CONTINUE signal in case the process was stopped
    if (kill(m_CurrentJobs[idx].GetPID(), SIGCONT) < 0)
    {
        return false;
    }

    if (bSendToForeground)
    {
        m_CurrentJobs[idx].SetExecType(ExecutionType::FOREGROUND);
        WaitForJob(idx);
    }
    else
    {
        m_CurrentJobs[idx].SetExecType(ExecutionType::BACKGROUND);
    }

    return true;
}

std::string Shell::GetAbsolutePath()
{

    char *path_buffer = realpath("/proc/self/exe", nullptr);
    if (path_buffer)
    {
        std::string path_str{path_buffer};
        free(path_buffer);
        return path_str;
    }
    else
    {
        perror(GetName().c_str());
    }
    return {};
}

std::string Shell::GetCurrWorkingDir()
{
    const size_t maxChunks = 25; // (25*FILENAME_MAX) = 100 KBs of current path are more than enough

    char stackBuffer[FILENAME_MAX]; // Stack buffer for the "normal" case
    if (getcwd(stackBuffer, sizeof(stackBuffer)) != NULL)
        return stackBuffer;
    if (errno != ERANGE)
    {
        // It's not ERANGE, so we don't know how to handle it
        perror(GetName().c_str());
        return {};
    }

    std::string cwd;
    // Ok, the stack buffer isn't long enough; fallback to heap allocation
    for (size_t chunks = 2; chunks < maxChunks; ++chunks)
    {
        const size_t bufSize = FILENAME_MAX * chunks;
        cwd.resize(bufSize);

        // starting with C++11 std::string is using contiguous memory
        // and it's safe to use  &cwd[0] to write on internal buffer directly
        if (getcwd(&cwd[0], bufSize) != NULL)
            return cwd;
        if (errno != ERANGE)
        {
            // It's not ERANGE, so we don't know how to handle it
            perror(GetName().c_str());
            return {};
        }
    }
    perror(GetName().c_str());
    return {};
}