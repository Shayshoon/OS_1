#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <deque>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h 

SmallShell::SmallShell() {
// TODO: add your implementation
    this->prompt = DEFAULT_PROMPT;
    this->jobs = new JobsList();
}

SmallShell::~SmallShell() {
// TODO: add your implementation
    delete this->jobs;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
    // For example:
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(WHITESPACE));

    if (firstWord == "chprompt") {
        return new ChangePromptCommand(cmd_line);
    } else if (firstWord == "jobs") {
        return new JobsCommand(cmd_line, this->jobs);
    }
    /*

    if (firstWord.compare("pwd") == 0) {
      return new GetCurrDirCommand(cmd_line);
    }
    else if (firstWord.compare("showpid") == 0) {
      return new ShowPidCommand(cmd_line);
    }
    else if ...
    .....
    else {
      return new ExternalCommand(cmd_line);
    }
    */
    return nullptr;
}

void SmallShell::setPrompt(std::string prompt) {
  this->prompt = prompt;
}

std::string SmallShell::getPrompt() {
  return this->prompt;
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    // TODO: fork for external commands
    // for example:
    Command* cmd = CreateCommand(cmd_line);
    if (cmd != nullptr) {
        cmd->execute();
    }

    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

ChangePromptCommand::ChangePromptCommand(const char *cmdLine): BuiltInCommand(cmdLine) {
    string cmd_s = _trim(string(cmdLine));
    int index = (int) cmd_s.find_first_of(WHITESPACE);
    if (index != -1) {
        string cmd_args = _trim(cmd_s.substr(index));
        this->newPrompt = cmd_args.substr(0, cmd_args.find_first_of(WHITESPACE));
    } else {
        this->newPrompt = DEFAULT_PROMPT;
    }
}

void ChangePromptCommand::execute() {
    SmallShell::getInstance().setPrompt(this->newPrompt);
}

void JobsCommand::execute() {
    this->jobs->removeFinishedJobs();
    this->jobs->printJobsList();
}

void JobsList::printJobsList() {
    if (!this->jobs->empty()) {
        for (pair<int, JobEntry> curr: *this->jobs) {
            cout << "[" << curr.first << "] " << curr.second.getCmd() << endl;
        }
    }
}

void JobsList::removeFinishedJobs() {
    for (auto it = this->jobs->begin(); it != this->jobs->end();) {
        int status;
        pid_t result = waitpid(it->second.getPid(), &status, WNOHANG);
        if (result == -1) {
            perror("smash error: waitpid failed");
        } else if (result > 0) {
            // Job finished
            it = this->jobs->erase(it);
        } else {
            ++it;
        }
    }
}

void JobsList::addJob(Command *cmd, bool isStopped) {
    pid_t pid = fork();

    if (pid == 0) {
        setpgrp();

        char *args[COMMAND_MAX_ARGS];
        int numArgs = _parseCommandLine(cmd->getCmd(), args);

        execv(args[0], args);
        perror("smash error: execv failed");
        exit(1);
    } else if (pid > 0) {
        // Parent process
        int jobId = this->jobs->empty() ? 1 : this->jobs->rbegin()->first + 1;
        JobEntry newJob(cmd, isStopped, jobId, pid);

        this->jobs->insert(pair<int, JobEntry>(jobId, newJob));
    } else {
        perror("smash error: fork failed");
    }
}

void JobsList::removeJobById(int jobId) {
    if (this->jobs->find(jobId) != this->jobs->end()) {
        this->jobs->erase(jobId);
    }
}

JobsList::JobsList() {
    this->jobs = new map<int,JobEntry>();
}

JobsList::~JobsList() {
    delete this->jobs;
}

void JobsList::killAllJobs() {
    for (pair<int, JobEntry> curr: *this->jobs) {
        if (curr.second.getIsStopped()) {
            kill(curr.second.getPid(), SIGCONT);
        }
        kill(curr.second.getPid(), SIGKILL);
    }
    this->jobs->clear();
}
