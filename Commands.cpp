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
#include <algorithm>
#include <sys/stat.h>
#include <cmath>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <cerrno>


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

bool _isNumber(char* s) {
    for (int i = 0; s[i] != '\0'; i++) {
        if (!isdigit(s[i])) {
            return false;
        }
    }
    return true;
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

void parseAliasPattern(const char* input, string& name, char*& command) {
    string line(input);

    size_t spacePos = line.find(' ');
    size_t eqPos = line.find('=');

    // Extract the alias (before '=')
    name = line.substr(spacePos + 1, eqPos - spacePos - 1);

    // Extract the command (after '='), strip quotes
    string command_str = _trim(line).substr(eqPos + 2, line.size() - eqPos - 3);

    // convert the command to char*
    command = strdup(command_str.c_str()); // Allocate char* memory
}

string getPwd() {
    char* buff = getcwd(nullptr, 0);
    string result;

    if(buff != nullptr){
        result = string(buff);
    }
    free(buff);

    return result;
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

    if (cmd_s.empty()) {
        return nullptr;
    }

    if (regex_match(cmd_s, regex("^[^|&]+\\|[^|&]+$|^[^|&]+(\\|&)[^|&]+$"))) {
        return new PipeCommand(cmd_line);
    }

    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(WHITESPACE));
    const char* thisAlias = this->aliases->getAlias(firstWord);

    if (regex_match(cmd_s, regex("^(([^>]+>[^>]+)|([^>]+>>[^>]+))$"))) {

        return new RedirectionCommand(cmd_line);
    }

    if (thisAlias != nullptr) {
        string aliasCmd = _trim(string(thisAlias))
                + cmd_s.substr(firstWord.length(), string::npos);
        return SmallShell::CreateCommand(aliasCmd.c_str());
    } else if (firstWord == "chprompt") {
        return new ChangePromptCommand(cmd_line);
    } else if (firstWord == "jobs") {
        return new JobsCommand(cmd_line, this->jobs);
    } else if (firstWord == "showpid"){
        return new ShowPidCommand(cmd_line);
    } else if (firstWord == "pwd") {
        return new GetCurrDirCommand(cmd_line);
    } else if (firstWord == "quit") {
        return new QuitCommand(cmd_line, this->getjobs());
    } else if (firstWord == "fg") {
        return new ForegroundCommand(cmd_line, this->getjobs());
    } else if (firstWord == "cd") {
        return new ChangeDirCommand(cmd_line,&lastDirectory);
    } else if (firstWord == "unsetenv"){
        return new UnSetEnvCommand(cmd_line);
    } else if (firstWord == "kill") {
        return new KillCommand(cmd_line, this->getjobs());
    } else if(firstWord == "watchproc"){
        return new WatchProcCommand(cmd_line);
    } else if (firstWord == "alias") {
        return new AliasCommand(cmd_line);
    } else if (firstWord == "unalias") {
        return new UnAliasCommand(cmd_line);
    } else if (firstWord == "du") {
        return new DiskUsageCommand(cmd_line);
   } else if (firstWord == "whoami") {
        return new WhoAmICommand(cmd_line);
   } else {
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
        cmd->setOriginalCmdLine(strdup(cmd_line));
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

AliasMap *SmallShell::getAliasMap() {
    return this->aliases;
}

JobsList::JobEntry * SmallShell::getForegroundProcess() {
    return this->foregroundProcess;
}

void SmallShell::setForegroundProcess(JobsList::JobEntry *fgProcess) {
    this->foregroundProcess = fgProcess;
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
            throw SysError("waitpid");
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

void AliasMap::addAlias(const std::string &name, const char *command) {
    for(auto& pair: this->aliases)
        if(pair.first == name)
            return;

    this->aliases.emplace_back(name, strdup(command));
}

int AliasMap::removeAlias(const std::string &name) {
   for (auto it = aliases.begin(); it != aliases.end(); ++it) {
        if (it->first == name) {
            free(it->second);
            aliases.erase(it);
            return 1;
        }
    }
    return 0;
}

const char *AliasMap::getAlias(const std::string &name) const {
    for (const auto& pair : this->aliases) {
        if (pair.first == name) {
            return pair.second;
        }
    }
    return nullptr;
}

void AliasMap::print() {
    for(const auto& pair: aliases){
        std::cout << pair.first << "='" << pair.second << "'" << std::endl;
    }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%-commands-%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


Command::Command(const char *cmd_line): cmd(cmd_line), originalCmdLine(nullptr) {
    this->args = new char*[COMMAND_MAX_ARGS];
    this->argsCount = _parseCommandLine(cmd_line, this->args);
    this->isBackground = _isBackgroundCommand(this->getCmd());

    char* str = strdup(cmd_line);

    if (this->isBackground) {
        _removeBackgroundSign(str);
    }

    this->cmd = strdup(cmd_line);
    this->argsCount = _parseCommandLine(str, this->args);
    delete[] str;
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

void ShowPidCommand::execute() {
    std::cout << SmallShell::getInstance().getPrompt() << " jobId is " << getpid() << std::endl;
}

void GetCurrDirCommand::execute() {
    string pwd = getPwd();
    if(!pwd.empty()){
        cout << pwd << endl;
    }
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd)
    : BuiltInCommand(cmd_line) {
    string cmd_s = _trim(string(cmd_line));
    char* args[COMMAND_MAX_ARGS];
    _parseCommandLine(cmd_line, args);
    if (args[2] != nullptr)//more then one arg
        throw SmashError("cd: too many arguments");
    if (args[1] != nullptr && strcmp(args[1], "-") == 0) {
        if (plastPwd == nullptr || *plastPwd == nullptr)//dont have last direction
            throw SmashError("cd: OLDPWD not set");
        else {
            char* temp = SmallShell::getInstance().getcurrDirectory();
            SmallShell::getInstance().setcurrDirectory(*plastPwd);
            SmallShell::getInstance().setLastDirectory(temp);
        }
    } else {
        if (args[1] != nullptr && strcmp(args[1], "..") == 0) {
            char *buff = getcwd(nullptr, 0);
            std::string trimmedPath;
            if (buff != nullptr) {
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
        throw SysError("cd failed");
    }
}

KillCommand::KillCommand(const char *cmd_line, JobsList *jobs): BuiltInCommand(cmd_line) {
    if (this->argsCount != 3
        || !_isNumber(this->args[2])
        || this->args[1][0] != '-'
        || !_isNumber(this->args[1]+1)) {
        throw SmashError("kill: invalid arguments");
    }

    int jobId = atoi(this->args[2]);
    this->signum = atoi(this->args[1] + 1);

    if(this->signum < 1 || this->signum > RANGEOFSIGNALS-1){
        throw SmashError("kill: invalid arguments");
    }

    JobsList::JobEntry* job = jobs->getJobById(jobId);
    if(job == nullptr){
        string errorMsg = "kill: job-id " + std::to_string(jobId) + " does not exist";
        throw SmashError(strdup(errorMsg.c_str()));
    }

    this->pid = job->getPid();
}

void KillCommand::execute() {
    if(kill(this->pid , this->signum) == -1){
        throw SysError("kill");
    }

    std::cout << "signal number " << this->signum << " was sent to pid " << this->pid << std::endl;
}

void JobsCommand::execute() {
    this->jobs->removeFinishedJobs();
    this->jobs->printJobsList();
}

void ExternalCommand::execute() {
    pid_t pid = fork();

    if (pid < 0) {
        throw SysError("fork");
    } else if (pid > 0) {
        // parent process
        if (this->isBackground) {
            SmallShell::getInstance().getjobs()->addJob(this, pid, false);
        } else {
            JobsList::JobEntry job = JobsList::JobEntry(nullptr, false, 0, pid);
            SmallShell::getInstance().setForegroundProcess(&job);

            if (waitpid(pid, nullptr, 0) == -1) {
                throw SysError("waitpid");
            }

            SmallShell::getInstance().setForegroundProcess(nullptr);
        }
    } else {
        // child process
        setpgrp();
        if (!this->isComplex) {
            execvp(this->args[0], this->args);
        } else {
            char* args[4];
            args[0] = (char*) "bash";
            args[1] = (char*) "-c";
            args[2] = (char*) this->getCmd();
            args[3] = nullptr;
            execvp(args[0], args);
        }

        // if execvp fails, it means external command does not exist
        throw SysError("execvp");
    }
}

QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs): BuiltInCommand(cmd_line) {
    this->kill = this->argsCount > 1 && strcmp(this->args[1], "kill") == 0;
    this->jobs = jobs;
}

void QuitCommand::execute() {
    if (this->kill) {
        cout << "smash: sending SIGKILL signal to "
            << this->jobs->getJobs()->size() << " jobs:" << endl;
        for (std::pair<const int, JobsList::JobEntry> job: *this->jobs->getJobs()) {
            cout << job.second.getPid() << ": " << job.second.getCmd() << endl;
        }
        SmallShell::getInstance().getjobs()->killAllJobs();
    }
    exit(0);
}

void ForegroundCommand::execute() {
    JobsList::JobEntry* job = this->jobs->getJobById(this->jobId);
    SmallShell::getInstance().setForegroundProcess(job);

    if (job->getIsStopped()) {
        kill(job->getPid(), SIGCONT);
        job->setIsStopped(false);
    }

    cout << job->getCmd() << " " << job->getPid() << endl;

    if (waitpid(job->getPid(), nullptr, 0) == -1) {
        throw SysError("waitpid");
    }

    SmallShell::getInstance().setForegroundProcess(nullptr);
    this->jobs->removeJobById(this->jobId);
}

ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs)
    : BuiltInCommand(cmd_line), jobs(jobs) {
    if (this->argsCount > 2) {
        throw SmashError("fg: invalid arguments");
    } if (this->argsCount == 2) {
        if (!_isNumber(this->args[1])) {
            throw SmashError("fg: invalid arguments");
        }

        this->jobId = atoi(this->args[1]);
        if (this->jobs->getJobById(this->jobId) == nullptr) {
            string errorMsg = "fg: job-id " + std::to_string(this->jobId) + " does not exist";
            throw SmashError(strdup(errorMsg.c_str()));
        }
    } else {
        if (this->jobs->getJobs()->empty()) {
            throw SmashError("fg: jobs list is empty");
        }

        this->jobId = this->jobs->getJobs()->rbegin()->first;
    }

}

UnSetEnvCommand::UnSetEnvCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {
    if (this->argsCount < 2) {
        throw SmashError("unsetenv: not enough arguments");
    }

    std::ostringstream oss;
    oss << "/proc/" << getpid() << "/environ";
    std::string path = oss.str();
    int fd = open(path.c_str(), O_RDONLY);

    if (fd == -1) {
        throw SysError("open");
    }
    char *buffer = (char *) malloc(4096);

    if (buffer == nullptr) {
        throw SysError("malloc");
    }
    ssize_t bytesRead = read(fd, buffer, 4096);
    buffer[bytesRead] = '\0';

    if (bytesRead == -1) {
        throw SysError("read");
    }

    string env(buffer, bytesRead);
    this->envVariables = split(env, '\0');

    for (auto & envVariable : this->envVariables) {
        string str = envVariable;
        envVariable = str.substr(0, str.find('='));
    }

    free(buffer);
    close(fd);

}


void _removeEnvVar(const char* var) {
    size_t len = strlen(var);
    for (char** env = environ; *env != nullptr; env++) {
        if (strncmp(*env, var, len) == 0 && (*env)[len] == '=') {
            char** newEnv = env;

            *newEnv = newEnv[1];
            newEnv++;
            while (*newEnv != nullptr) {
                *newEnv = newEnv[1];
                newEnv++;
            }
            return;
        }
    }
}

bool UnSetEnvCommand::doesEnvVarExist(const char* var) {
    bool found = false;
    for (auto str = this->envVariables.begin();
         str != this->envVariables.end() && !found; ++str) {
        found |= strcmp(str->data(), var) == 0;
    }

    return found;
}

void UnSetEnvCommand::execute() {
//    check if the args are valid
    for (int i = 1; i < this->argsCount; i++) {
        if (!this->doesEnvVarExist(this->args[i])) {
            string errorMsg = "unsetenv: " + string(this->args[i]) + " does not exist";
            throw SmashError(strdup(errorMsg.c_str()));
        }
    }

//    unset env variables using __environ array
    for (int i = 1; i < this->argsCount; i++) {
        for (int envIndex = 0; envIndex < (int) this->envVariables.size(); envIndex++) {
            _removeEnvVar(this->args[i]);
        }
    }
}

AliasCommand::AliasCommand(const char *cmd_line): BuiltInCommand(cmd_line) {
    this->cmdLine = nullptr;

    if(!(this->print = this->argsCount < 2)) {
        regex pattern("^alias [a-zA-Z0-9_]+='[^']*'$");

        if (!regex_match(string(cmd_line), pattern)) {
            throw SmashError("alias: invalid alias format");
        }
        parseAliasPattern(this->getCmd(), this->aliasName, this->cmdLine);
        AliasMap *aliases = SmallShell::getInstance().getAliasMap();

        if (aliases->getAlias(this->aliasName) != nullptr
            || aliases->getShellKeywords().count(this->aliasName) > 0) {

            string errorMsg =
                    "alias: " + this->aliasName + " already exists or is a reserved command";
            throw SmashError(strdup(errorMsg.c_str()));
        }
    }
}

void AliasCommand::execute() {
    AliasMap* aliases = SmallShell::getInstance().getAliasMap();
    if(this->print){
        aliases->print();
    } else{
        aliases->addAlias(this->aliasName , this->cmdLine);
    }
}

void AliasCommand::createAlias(char *cmd) {
    SmallShell::getInstance().CreateCommand(cmd);
}

UnAliasCommand::UnAliasCommand(const char *cmd_line): BuiltInCommand(cmd_line) {
    char* args[COMMAND_MAX_ARGS];
    int count = _parseCommandLine(cmd_line ,args);
    AliasMap* aliases = SmallShell::getInstance().getAliasMap();
    if (args[1] == nullptr){
        throw SmashError("unalias: not enough arguments");
    }
    for (int i=1 ; i<count ; i++){
        if (aliases->getAlias(string(args[i])) == nullptr){
            string errorMsg = "unalias: " + string(args[i]) + " alias does not exist";
            throw SmashError(strdup(errorMsg.c_str()));
        }
        unAlias.push_back(string(args[i]));
    }
}

void UnAliasCommand::execute() {
    AliasMap* aliases = SmallShell::getInstance().getAliasMap();
    for (const string& name : unAlias){
        aliases->removeAlias(name);
    }
}


WatchProcCommand::WatchProcCommand(const char *cmd_line)
        : BuiltInCommand(cmd_line), cpuUsage(0.0), memoryUsage(""),
          pidProcess(""){
    char* args[COMMAND_MAX_ARGS];
    int count = _parseCommandLine(cmd_line ,args);
    if(count != 2 || !(_isNumber(args[1]))){
        throw SmashError("watchproc: invalid arguments");
    }
    int pid = atoi(args[1]);
    if(kill(pid , 0) == 0) {
        this->pidProcess = args[1];
    } else{
        string errorMsg = "watchproc: jobId " + string(args[1]) + " does not exist";
        throw SmashError(strdup(errorMsg.c_str()));
    }
}

void WatchProcCommand::execute() {
//      Read stat file
    string statPath = "/proc/" + this->pidProcess + "/stat";
    int fd = open(statPath.c_str() , O_RDONLY);
    if (fd == -1) {
        throw SysError("open");
    }
    char* buf = new char[BUFF_SIZE];
    ssize_t bytesRead = read(fd , buf , BUFF_SIZE - 1);
    close(fd);
    buf[bytesRead] = '\0';

    string statContent = string(buf);


//                extract relevant data
    vector<string> info = split(statContent , ' ');
    long utime = stol(info[UTIME]);
    long stime = stol(info[STIME]);
    long starttime = stol(info[STARTTIME]);
    long clk_tck = sysconf(_SC_CLK_TCK);

//    read uptime file
    fd = open("/proc/uptime" , O_RDONLY);
    if (fd == -1) {
        throw SysError("open");
    }
    read(fd , buf , BUFF_SIZE - 1);
    close(fd);
    buf[bytesRead] = '\0';

    string uptimeContent = string(buf);
    vector<string> info_time = split(uptimeContent , ' ');
    double uptime = stol(info_time[0]);

    double seconds = uptime - (starttime / (double) clk_tck);
    this->cpuUsage = 100.0 * ((utime + stime) / (double) clk_tck) / seconds;

//    read status file
    string statusPath = "/proc/" + this->pidProcess + "/status";
    fd = open(statusPath.c_str() , O_RDONLY);
    if (fd == -1) {
        throw SysError("open");
    }
    bytesRead = read(fd , buf, BUFF_SIZE - 1);
    close(fd);

    if (bytesRead <= 0) {
        delete[] buf;
        return;
    }
    buf[bytesRead] = '\0';

    char* line = strtok(buf, "\n");
    while (line) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            string result = string(&line[10]);
            result = result.substr(0, result.size() - 3);
            this->memoryUsage = result;
        }
        line = strtok(nullptr, "\n");
    }

    delete[] buf;

    cout << "PID: " << this->pidProcess << " | CPU Usage: " << this->cpuUsage
         << "% | Memory Usage: " << stod(this->memoryUsage) / 1024 << " MB" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%-special command-%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

RedirectionCommand::RedirectionCommand(const char *cmd_line): Command(cmd_line) , outputFile(-1) {
    int flag = O_CREAT | O_WRONLY;
    bool appendMode;

    string cmd_s = _trim(string(cmd_line));
    appendMode = cmd_s.find(">>") != string::npos;
    flag |= appendMode ? O_APPEND : O_TRUNC;

    vector<string> parts = split(cmd_s, '>');

    string path = _trim(parts.rbegin()->data());
    char* path_c = strdup(path.c_str());
    if (_isBackgroundCommand(path_c)) {
        _removeBackgroundSign(path_c);
        path = string(path_c);
    }
    delete path_c;
    path = path.substr(0, path.find_first_of(WHITESPACE));
    string cmd = _trim(parts.front());

    this->outputFile = open(path.c_str(), flag, 0666);

    if (outputFile == -1) {
        throw SysError("open");
    }

    this->cmdToRun = string(cmd_line);
    this->cmdToRun = this->cmdToRun.substr(0, this->cmdToRun.find_first_of('>'));
}

void RedirectionCommand::execute() {
    int stdoutCopy = dup(STDOUT_FILENO);
    if (stdoutCopy == -1) {
        throw SysError("dup");
    }

    if (dup2(this->outputFile, STDOUT_FILENO) == -1) {
        close(this->outputFile);
        throw SysError("dup2");
    }
    close(this->outputFile);

    Command* cmd = SmallShell::getInstance().CreateCommand(this->cmdToRun.c_str());
    cmd->execute();

    if (dup2(stdoutCopy, STDOUT_FILENO) == -1) {
        close(stdoutCopy);
        throw SysError("dup2");
    }

    close(stdoutCopy);
}

long getBlocksOfFile(const string& path) {
    struct stat sb {};
    if (stat(path.c_str(), &sb) == -1) {
        if (errno == ENOENT) {
            string errorMsg = "du: directory " + path + " does not exist";
            throw SmashError(strdup(errorMsg.c_str()));
        }
        throw SysError("stat");
    }
    return sb.st_blocks;
}

long getBlocksOfDirectory(const string& path) {
    int fd = open(path.c_str(), O_RDONLY | O_DIRECTORY);
    if (fd == -1) {
        SysError("open");
    }

    int nread;
    char buf[BUFF_SIZE];
    long totalBlocks = 0;
    struct dirent *d;

    while ((nread = syscall(SYS_getdents64, fd, buf, BUFF_SIZE)) != 0) {
        if (nread == -1) {
            close(fd);
            SysError("getdents64");
        }
        for (int bpos = 0; bpos < nread; bpos += d->d_reclen) {
            d = (struct dirent *) (buf + bpos);
            string name = d->d_name;

            // Skip "." and ".." directories
            if (name == "." || name == "..") {
                continue;
            }

            // Construct the full path to the file or directory
            string full_path = path + "/" + name;

            // check if it is a directory using stat
            struct stat sb;
            if (stat(full_path.c_str(), &sb) == -1) {
                SysError("stat");
                exit(1);
            }

            if (sb.st_mode & S_IFDIR) {
                totalBlocks += getBlocksOfDirectory(full_path);
            }

            totalBlocks += getBlocksOfFile(full_path);
        }

    }

    close(fd);

    return totalBlocks;
}

void DiskUsageCommand::execute() {
    // Calculate the total disk usage in KB
    double totalKB = 0.5 * getBlocksOfDirectory(path);  // Assuming each block is 512 bytes (0.5 KB)
    std::cout << "Total disk usage: " << std::ceil(totalKB) << " KB" << std::endl;
}

void WhoAmICommand::execute() {
    cout << this->username << " " << getPwd() << endl;
}

WhoAmICommand::WhoAmICommand(const char *cmd_line) : Command(cmd_line) {
    uid_t uid = getuid();

//    open /etc/passwd file using only syscalls
    int fd = open("/etc/passwd", O_RDONLY);
    if (fd == -1) {
        SysError("open");
    }

    char* buffer = new char[BUFF_SIZE];
    ssize_t bytesRead = read(fd, buffer, BUFF_SIZE - 1);
    close(fd);

    if (bytesRead <= 0) {
        delete[] buffer;
        return;
    }
    buffer[bytesRead] = '\0';
    string passwdContent(buffer);
    delete[] buffer;

    vector<string> lines = split(passwdContent, '\n');

    for (const string &line : lines) {
        if (!line.empty()) {
            vector<string> fields = split(line, ':');
            if (fields.size() > 2 && stoi(fields[2]) == (int) uid) {
                this->username = fields[0];
                break;
            }
        }
    }
}
PipeCommand::PipeCommand(const char *cmd_line) : Command(cmd_line) {
    string cmd_s = string(cmd_line);
    int pipeIndex = cmd_s.find('|');
    this->sender = _trim(cmd_s.substr(0, pipeIndex));
    this->forwardErrors = (cmd_s[pipeIndex + 1] == '&');

    if (this->forwardErrors) {
        this->receiver = _trim(cmd_s.substr(pipeIndex + 2, string::npos));
    } else {
        this->receiver = _trim(cmd_s.substr(pipeIndex + 1, string::npos));
    }
}

void PipeCommand::execute() {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        throw SysError("pipe");
    }

    pid_t pid = fork();
    if (pid == -1) {
        throw SysError("fork");
    } else if (pid == 0) {
        // Child process for receiver
        if (close(pipefd[1]) == -1) {
            throw SysError("close");
        }
        if (dup2(pipefd[0], STDIN_FILENO) == -1) {
            throw SysError("dup2");
        }
        if (close(pipefd[0]) == -1) {
            throw SysError("close");
        }
        Command* cmd = SmallShell::getInstance().CreateCommand(this->receiver.c_str());
        cmd->execute();
        exit(0);
    } else {
        int fileno = this->forwardErrors ? STDERR_FILENO : STDOUT_FILENO;
        int stdout_fd = dup(fileno);
        if (stdout_fd == -1) {
            throw SysError("dup");
        }
        if (dup2(pipefd[1], fileno) == -1) {
            throw SysError("dup2");
        }
        if (close(pipefd[1]) == -1) {
            throw SysError("close");
        }
        if (close(pipefd[0]) == -1) {
            throw SysError("close");
        }

        Command* cmd = SmallShell::getInstance().CreateCommand(this->sender.c_str());
        cmd->execute();

        if (dup2(stdout_fd, fileno) == -1) {
            throw SysError("dup2");
        }
        if (close(stdout_fd) == -1) {
            throw SysError("close");
        }
    }

    close(pipefd[0]);
    close(pipefd[1]);

    waitpid(pid, nullptr, 0);
}
