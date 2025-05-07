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

    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(WHITESPACE));
    const char* thisAlias = this->aliases->getAlias(firstWord);

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
    kill(pid , signum);
}

void JobsCommand::execute() {
    this->jobs->removeFinishedJobs();
    this->jobs->printJobsList();
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
        perror("smash error: execvp failed");
        exit(1);
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

    if (job->getIsStopped()) {
        kill(job->getPid(), SIGCONT);
        job->setIsStopped(false);
    }

    cout << job->getCmd() << " " << job->getPid() << endl;

    if (waitpid(job->getPid(), nullptr, 0) == -1) {
        perror("smash error: waitpid failed");
    }

    this->jobs->removeJobById(this->jobId);
}

ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs)
    : BuiltInCommand(cmd_line), jobs(jobs) {
    if (this->argsCount > 2) {
        cerr << "smash error: fg: invalid arguments" <<  endl;
        return;
    } if (this->argsCount == 2) {
        if (!_isNumber(this->args[1])) {
            cerr << "smash error: fg: invalid arguments" <<  endl;
            return;
        }

        this->jobId = atoi(this->args[1]);
        if (this->jobs->getJobById(this->jobId) == nullptr) {
            cerr << "smash error: fg: job-id " << this->jobId << " does not exist" << endl;
            return;
        }
    } else {
        if (this->jobs->getJobs()->empty()) {
            cerr << "smash error: fg: jobs list is empty" << endl;
            return;
        }

        this->jobId = this->jobs->getJobs()->rbegin()->first;
    }

}

UnSetEnvCommand::UnSetEnvCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {
    if (this->argsCount < 2) {
        cerr << "smash error: unsetenv: not enough arguments" << endl;
    }

    std::ostringstream oss;
    oss << "/proc/" << getpid() << "/environ";
    std::string path = oss.str();
    int fd = open(path.c_str(), O_RDONLY);

    if (fd == -1) {
        cerr << "smash error: open failed" << endl;
    } else {
        char *buffer = (char *) malloc(4096);

        if (buffer == nullptr) {
            perror("smash error: malloc failed");
        } else {
            ssize_t bytesRead = read(fd, buffer, 4096);
            buffer[bytesRead] = '\0';

            if (bytesRead == -1) {
                perror("smash error: read failed");
            } else {
                string env(buffer, bytesRead);
                this->envVariables = split(env, '\0');

                for (auto & envVariable : this->envVariables) {
                    string str = envVariable;
                    envVariable = str.substr(0, str.find('='));
                }
            }
            free(buffer);
        }
        close(fd);
    }
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
            cerr << "smash error: unsetenv: " << this->args[i] << " does not exist" << endl;
            continue;
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

        if (regex_match(string(cmd_line), pattern)) {
            parseAliasPattern(this->getCmd(), this->aliasName, this->cmdLine);
            AliasMap* aliases = SmallShell::getInstance().getAliasMap();

            if (aliases->getAlias(this->aliasName) != nullptr
                || aliases->getShellKeywords().count(this->aliasName) > 0) {

                cerr << "smash error: aliases: " << aliases
                     << " already exists or is a reserved command" << endl;
                return;
            }
        } else {
            cerr << "smash error: alias: invalid alias format" << endl;
            return;
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
        cout << "smash error: unalias: not enough arguments" << endl;
        return;
    }
    for (int i=1 ; i<count ; i++){
        if (aliases->getAlias(string(args[i])) == nullptr){
            cout << "smash error: unalias: " << string(args[i]) << " alias does not exist" << endl;
            return;
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
          pidProcess(""), isValid(false){
    char* args[COMMAND_MAX_ARGS];
    int count = _parseCommandLine(cmd_line ,args);
    if(count != 2 || !(_isNumber(args[1]))){
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
    if (!this->isValid) {
        perror("error"); // TODO: write meaningful message
        return;
    }

//      Read stat file
    string statPath = "/proc/" + this->pidProcess + "/stat";
    int fd = open(statPath.c_str() , O_RDONLY);
    if (fd == -1) {
        perror("smash error: open failed");
        return;
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
        perror("smash error: open failed");
        return;
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
        perror("smash error: open failed");
        return;
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

long getBlocksOfFile(const string& path) {
    struct stat sb {};
    if (stat(path.c_str(), &sb) == -1) {
        if (errno == ENOENT) {
            cerr << "smash error: du: directory " << path << " does not exist" << endl;
        }
        perror("smash error: stat failed");
        return -1;
    }
    return sb.st_blocks;
}

//void DiskUsageCommand::execute() {
////    iterate recursively over the directory using getdents syscall only
//    struct linux_dirent* dirent = new linux_dirent[BUFF_SIZE];
//    int fd = open(this->path.c_str(), O_RDONLY | O_DIRECTORY);
//    getdents64(fd, );
//
//
//    cout << "Total disk usage: " << std::ceil(0.5 * double(getBlocksOfFile(this->path))) << " KB" << endl;
//}
void DiskUsageCommand::execute() {
    int                  fd;
    char                 d_type;
    char                 buf[BUFF_SIZE];
    long                 nread;
    struct dirent  *d;

    long totalBlocks = 0;

    fd = open(this->argsCount > 1 ? this->args[1] : ".", O_RDONLY | O_DIRECTORY);
    if (fd == -1) {
        cerr << "open";
        exit(1);
    }

    while (true) {
        nread = syscall(SYS_getdents64, fd, buf, BUFF_SIZE);
        if (nread == -1) {
            perror("getdents64");
            exit(1);
        }
        if (nread == 0) {
            break;
        }

        for (size_t bpos = 0; bpos < (size_t) nread;) {
            d = (struct dirent *) (buf + bpos);
            std::string name = d->d_name;

            // Skip "." and ".." directories
            if (name == "." || name == "..") {
                bpos += d->d_reclen;
                continue;
            }

            // Construct the full path to the file or directory
            std::string full_path = this->path + "/" + name;

            // Get the disk usage (blocks) for this file/directory
            long blocks = getBlocksOfFile(full_path);
            if (blocks != -1) {
                std::cout << name << ": " << blocks << " blocks" << std::endl;
                totalBlocks += blocks;  // Add to total
            }

            bpos += d->d_reclen;
        }
    }

close(fd);

// Calculate the total disk usage in KB
double totalKB = 0.5 * totalBlocks;  // Assuming each block is 512 bytes (0.5 KB)
std::cout << "Total disk usage: " << std::ceil(totalKB) << " KB" << std::endl;

}

