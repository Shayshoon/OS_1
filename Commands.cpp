
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <deque>
#include "Commands.h"
#include <signal.h>
#include <map>
#include <regex>
#include <cstring>
#include <fcntl.h>

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

bool _isBackgroundCommand(const char *cmd_line) {
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

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        result.push_back(token);
    }
    return result;
}

void parseAliasPattern(const char* input, std::string& name, char*& command) {
    std::string line(input);

    size_t eq_pos = line.find('=');
    if (eq_pos == std::string::npos) {
        cout << "invalid pattern parseAliasPatrern" << endl;
    }

    // Extract the name (before '=')
    name = line.substr(0, eq_pos);

    // Extract the command (after '='), strip quotes
    std::string cmd = line.substr(eq_pos + 1);

    // Remove the surrounding quotes and convert the command to char*
    std::string command_str = cmd.substr(1, cmd.size() - 2); // Strip the quotes
    command = strdup(command_str.c_str()); // Allocate char* memory
}

bool isInteger(const char* number) {
    if (number == nullptr) return false;
    for (int i = 0; number[i]; ++i) {
        if (!isdigit(number[i]))
            return false;
    }
    return true;
}

string readFile(const string& path) {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        return "";
    }

    char buf[4096];
    ssize_t bytes = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (bytes <= 0) {
        return "";
    }

    buf[bytes] = '\0';
    return string(buf);
}

// TODO: Add your implementation for classes in Commands.h 

//%%%%%%%%%%%%%%%%%%%%%%%%%-SmallShell-%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

SmallShell::SmallShell() {
    lastDirectory = nullptr;
    currDirectory = getcwd(NULL, 0);
    this->prompt = DEFAULT_PROMPT;
    this->jobs = new JobsList();
    this->aliases = new AliasMap();
}

SmallShell::~SmallShell() {
// TODO: add your implementation
    delete this->jobs;
    delete this->aliases;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
    // For example:
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(WHITESPACE));
    AliasMap* aliases = SmallShell::getAliasMap();
    const char* thisAlias = aliases->getAlias(firstWord);

    if (firstWord == "chprompt") {
        return new ChangePromptCommand(cmd_line);
    } else if (firstWord == "jobs") {
        return new JobsCommand(cmd_line, this->jobs);
    } else if (firstWord == "showpid"){
        return new ShowPidCommand(cmd_line);
    } else if (firstWord == "pwd"){
        return new GetCurrDirCommand(cmd_line);
    } else if (firstWord == "cd"){
        return new ChangeDirCommand(cmd_line,&lastDirectory);
    } else if (firstWord == "kill"){
        return new KillCommand(cmd_line , SmallShell::getInstance().getjobs());
    } else if (firstWord == "alias"){
        return new AliasCommand(cmd_line);
    }else if(firstWord == "unalias"){
        return new UnAliasCommand(cmd_line);
    }else if(firstWord == "watchproc"){
        return new WatchProcCommand(cmd_line);
    }
    else if(thisAlias != nullptr){
        return SmallShell::CreateCommand(thisAlias);
        }
    else{
        return new ExternalCommand(cmd_line);
    }

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
        this->getjobs()->removeFinishedJobs();
        cmd->execute();
    }

    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

char* SmallShell::getLastDirectory() {
    return lastDirectory;
}

void SmallShell::setLastDirectory(char* dir) {
    if (lastDirectory) free(lastDirectory);
    lastDirectory = strdup(dir);
}

char* SmallShell::getcurrDirectory() {
    return currDirectory;
}

void SmallShell::setcurrDirectory(char* dir) {
    if (currDirectory) {
        free(currDirectory);
    }
    currDirectory = strdup(dir);
}

JobsList *SmallShell::getjobs() {
    return this->jobs;
}

AliasMap* SmallShell::getAliasMap() {
    return  this->aliases;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%-JobsList-%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

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

void JobsList::addJob(Command *cmd, pid_t pid, bool isStopped) {
    this->removeFinishedJobs();

    int jobId = this->jobs->empty() ? 1 : this->jobs->rbegin()->first + 1;
    JobEntry newJob(cmd, isStopped, jobId, pid);

    this->jobs->insert(pair<int, JobEntry>(jobId, newJob));
}

void JobsList::removeJobById(int jobId) {
    if (this->jobs->find(jobId) != this->jobs->end()) {
        this->jobs->erase(jobId);
    }
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

JobsList::JobEntry* JobsList::getJobById(int jobId) {
    auto it = jobs->find(jobId);
    if (it != jobs->end()) {
        return &(it->second); // return pointer to the JobEntry stored in the map
    }
    return nullptr;
}

JobsList::JobEntry *JobsList::getLastJob(int *lastJobId) {
    return nullptr;
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId) {
    return nullptr;
}

JobsList::JobsList() {
    this->jobs = new map<int,JobEntry>();
}

JobsList::~JobsList() {
    delete this->jobs;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%-Alias-%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int AliasMap::addAlias(const std::string &name, const char *command) {
    for(auto& pair:myAlias)
        if(pair.first == name)
            return 0;
    myAlias.emplace_back(name, strdup(command));
    return 1;
}

int AliasMap::removeAlias(const std::string &name) {
   for (auto it = myAlias.begin(); it != myAlias.end(); ++it) {
        if (it->first == name) {
            delete(it->second);
            myAlias.erase(it);
            return 1;
        }
    }
    return 0;
}

const char *AliasMap::getAlias(const std::string &name) const {
    for (const auto& pair : myAlias) {
        if (pair.first == name) {
            return pair.second;
        }
    }
    return nullptr;
}

void AliasMap::print() {
    for(const auto& pair: myAlias){
        std::cout << pair.first << "='" << pair.second << "'" << std::endl;
    }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%-commands-%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

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

void ShowPidCommand::execute() {
    std::cout << SmallShell::getInstance().getPrompt() << " pid is " << getpid() << std::endl;
}

void GetCurrDirCommand::execute() {
    char* buff = getcwd(NULL, 0);
    if(buff != NULL){
        std::cout << buff << std::endl;
        free(buff);
    }
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd)
    : BuiltInCommand(cmd_line) {
    string cmd_s = _trim(string(cmd_line));
    char* args[COMMAND_MAX_ARGS];
    _parseCommandLine(cmd_line, args);
    if (args[2] != nullptr)//more then one arg
        std::cerr << "smash error: cd: too many arguments" << std::endl;
    if (args[1] != nullptr && strcmp(args[1], "-") == 0) {
        if (plastPwd == nullptr || *plastPwd == nullptr)//dont have last direction
            std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
        else {
            char* temp = SmallShell::getInstance().getcurrDirectory();
            SmallShell::getInstance().setcurrDirectory(*plastPwd);
            SmallShell::getInstance().setLastDirectory(temp);
        }
    } else {
        if (args[1] != nullptr && strcmp(args[1], "..") == 0) {
            char *buff = getcwd(NULL, 0);
            std::string trimmedPath;
            if (buff != NULL) {
                std::string fullPath(buff);
                std::vector<std::string> parts = split(fullPath, '/');

                if (!parts.empty()) {
                    parts.pop_back();
                }

                for (const auto &part: parts) {
                    if (!part.empty()) {
                        trimmedPath += "/" + part;
                    }
                }
            }

            if (trimmedPath.empty()) {
                trimmedPath = "/";
            }
            SmallShell::getInstance().setLastDirectory(SmallShell::getInstance().getcurrDirectory());
            SmallShell::getInstance().setcurrDirectory(strdup(trimmedPath.c_str()));
        } else {
            *plastPwd = strdup(SmallShell::getInstance().getcurrDirectory());
            SmallShell::getInstance().setLastDirectory(SmallShell::getInstance().getcurrDirectory());
            SmallShell::getInstance().setcurrDirectory(args[1]);
        }
    }
}

void ChangeDirCommand::execute() {
    const char *path = SmallShell::getInstance().getcurrDirectory();
    if (chdir(path) == -1) {
        perror("smash error: cd failed");
    }
}

KillCommand::KillCommand(const char *cmd_line, JobsList *jobs): BuiltInCommand(cmd_line) {
    char* args[COMMAND_MAX_ARGS];
    int count = _parseCommandLine(cmd_line ,args);
    char* end;
     this->pid = strtoul(args[2], &end, 10);
    //int signum;
    if (args[1] != nullptr && args[1][0] == '-') {
        this->signum = atoi(args[1] + 1);  // Skip the first character (-) and convert to int
    }
    if(count != NUMBEROFSIGNALS && *end == '\0') {
        std::cout << "smash error: kill: invalid arguments" << std::endl;
        return;
    }
    if(this->signum < 1 || this->signum > RANGEOFSIGNALS-1){
        perror("smash error: kill failed");
        return;
    }
    if(jobs->getJobById(this->pid) == nullptr){
        std::cout << "smash error: kill: job-id <" << this->pid << "> does not exist" << std::endl;
        return;
    }
    std::cout << "signal number " << this->signum << "was sent to pid " << this->pid << std::endl;
}

void KillCommand::execute() {
   /* JobsList* jobs = SmallShell::getInstance().getjobs();
    JobsList::JobEntry* job = jobs->getJobById(pid);
    job->setSignal(signum);*/
    kill(pid , signum);
}

void JobsCommand::execute() {
    this->jobs->removeFinishedJobs();
    this->jobs->printJobsList();
}

ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line) {
    this->isBackground = _isBackgroundCommand(this->cmd);
    if (this->isBackground) {
        char* str = strdup(cmd_line);
        _removeBackgroundSign(str);
        this->cmd = str;
        this->argsCount = _parseCommandLine(this->cmd, this->args);
    }
}

void ExternalCommand::execute() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("smash error: fork failed");
    } else if (pid > 0) {
        // parent process
        if (this->isBackground) {
            SmallShell::getInstance().getjobs()->addJob(this, pid, false);
        } else if (waitpid(pid, nullptr, 0) == -1) {
            perror("smash error: waitpid failed");
        }
    } else {
        // child process
        setpgrp();
        execvp(this->args[0], this->args);

        // if execvp fails, it means external command does not exist
        perror("smash error: execvp failed");
        exit(1);
    }
}

AliasCommand::AliasCommand(const char *cmd_line): BuiltInCommand(cmd_line) {
    this->shouldPrint = 0;
    this->name_command = nullptr;
    char* args[COMMAND_MAX_ARGS];
   // int count = _parseCommandLine(cmd_line ,args);
    if(std::string(args[0]) == "alias") {
        if(args[1] == nullptr){
            this->shouldPrint = 1;
        }
        else {
            std::regex pattern("^alias [a-zA-Z0-9_]+='[^']*'$");
            if (std::regex_match(string(cmd_line), pattern)) {
                parseAliasPattern(args[1], this->name, this->name_command);
                AliasMap* alias = SmallShell::getInstance().getAliasMap();
                if(alias->getAlias(this->name) != nullptr || alias->getShellKeywords().count(this->name)) {
                    std::cout << "smash error: alias: " << name << " already exists or is a reserved command"
                              << std::endl;
                    return;
                }
            } else {
                std::cout << "smash error: alias: invalid alias format" << std::endl;
                return;
            }
        }
    }


}

void AliasCommand::execute() {
    AliasMap* alias = SmallShell::getInstance().getAliasMap();
    if(this->shouldPrint == 1){
        alias->print();
    } else{
        if(!alias->addAlias(this->name , this->name_command))
            std::cout << "can not add" << std::endl;
    }
}

void AliasCommand::createAlias(char *cmd) {
    SmallShell::getInstance().CreateCommand(cmd);
}

UnAliasCommand::UnAliasCommand(const char *cmd_line): BuiltInCommand(cmd_line) {
    char* args[COMMAND_MAX_ARGS];
    int count = _parseCommandLine(cmd_line ,args);
    AliasMap* aliases = SmallShell::getInstance().getAliasMap();
    if(args[1] == nullptr){
        std::cout << "smash error: unalias: not enough arguments" << std::endl;
        return;
    }
    for(int i=1 ; i<count ; i++){
        if(aliases->getAlias(string(args[i])) == nullptr){
            std::cout << "smash error: unalias: <" << string(args[i]) << "> alias does not exist" << std::endl;
            return;
        }
        unAlias.push_back(std::string(args[i]));
    }
}

void UnAliasCommand::execute() {
    AliasMap* aliases = SmallShell::getInstance().getAliasMap();
    for (const std::string& name : unAlias){
        aliases->removeAlias(name);
    }
}

WatchProcCommand::WatchProcCommand(const char *cmd_line): BuiltInCommand(cmd_line) , cpuUsage(0.0), memoryUsage(""),
pidProcess("") , isValid(false){
    char* args[COMMAND_MAX_ARGS];
    int count = _parseCommandLine(cmd_line ,args);
    if(count != 2 || !(isInteger(args[1]))){
        cout << "smash error: watchproc: invalid arguments" << endl;
        return;
    }
    int pid = atoi(args[1]);
    if(kill(pid , 0) == 0) {
        this->pidProcess = args[1];
        this->isValid = true;
    } else{
        cout << "smash error: watchproc: pid <" << args[1] << "> does not exist" << endl;
        return;
    }
}

void WatchProcCommand::execute() {
        if (this->isValid) {
            string stat_path = "/proc/" + this->pidProcess + "/stat";
            const char* thePathToStat = stat_path.c_str();
            int descriptor = open(thePathToStat , O_RDONLY);
            if(descriptor != -1){
                string buf[BUFF_SIZE];
                while (read(descriptor , buf , READ_SIZE) != 0){}
                string content_buf;
                for (int i = 0; i < BUFF_SIZE; ++i) {
                    content_buf += buf[i];
                }
                vector<string> info = split(content_buf , ' ');
                long utime = stol(info[UTIME]);
                long stime = stol(info[STIME]);
                long starttime = stol(info[STARTTIME]);
                long clk_tck = sysconf(_SC_CLK_TCK);
                int fd = open("/proc/uptime" , O_RDONLY);
                string buf_time[BUFF_SIZE];
                if(fd != -1){
                    while ((read(fd , buf_time , READ_SIZE))){}
                }
                close(fd);
                string content;
                for (int i = 0; i < BUFF_SIZE; ++i) {
                    content += buf_time[i];
                }
                vector<string> info_time = split(content , ' ');
                double uptime = stol(info_time[0]);

                double seconds = uptime - (starttime / (double) clk_tck);
                double cpu_usage = 100.0 * ((utime + stime) / (double) clk_tck) / seconds;
                this->cpuUsage = cpu_usage;
                close(descriptor);
            }
            string status_path = "/proc/" + this->pidProcess + "/status";
            const char* thePathToStatus = status_path.c_str();

            int descriptor_two = open(thePathToStatus , O_RDONLY);
            if (descriptor_two == -1) {
                return;
            }

            char* buffer = (char*)malloc(BUFF_SIZE);
            if (!buffer) {
                close(descriptor_two);
                return;
            }

            ssize_t bytes = read(descriptor_two , buffer, BUFF_SIZE - 1);
            close(descriptor_two);
            if (bytes <= 0) {
                free(buffer);
                return;
            }

            buffer[bytes] = '\0';

            char* line = strtok(buffer, "\n");
            while (line) {
                if (strncmp(line, "VmRSS:", 6) == 0) {
                    char* result = strdup(line);
                    free(buffer);
                    this->memoryUsage = result;
                }
                line = strtok(nullptr, "\n");
            }

            free(buffer);
            close(descriptor_two);
            cout << "PID:" << this->pidProcess << " | CPU Usage: " << this->cpuUsage << " | Memory Usage: "
                 << this->memoryUsage << endl;
        }

}

