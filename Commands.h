// Ver: 10-4-2025
#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <map>
#include <cstring>
#include <cstdlib>
#include <set>

#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define DEFAULT_PROMPT "smash"
#define NUMBEROFSIGNALS (3)
#define RANGEOFSIGNALS (32)
#define MINIMUM_FOR_CALCULATE_CPUUSAGE (22)
#define UTIME (12)
#define STIME (13)
#define STARTTIME (21)
#define BUFF_SIZE (4096)

int _parseCommandLine(const char *cmd_line, char **args);

class Command {
    // TODO: Add your data members
protected:
    int argsCount;
    char** args;
    const char* cmd;
    const char* originalCmdLine;
public:
    Command(const char *cmd_line): cmd(cmd_line), originalCmdLine(nullptr) {
        this->args = (char**) malloc(sizeof(char*) * COMMAND_MAX_ARGS);
        if (args == nullptr) {
            perror("smash error: malloc failed");
            exit(1);
        }

        this->argsCount = _parseCommandLine(cmd_line, args);
    }

    virtual ~Command() {
        delete[] originalCmdLine;
        delete[] args;
    }

    virtual void execute() = 0;

    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed

    const char* getCmd() {
        return this->cmd;
    }

    const char* getOriginalCmdLine() {
        return this->originalCmdLine;
    }
    void setOriginalCmdLine(const char* cmd_line) {
        if (this->originalCmdLine == nullptr) {
            this->originalCmdLine = cmd_line;
        }
    }
};

std::vector<std::string> split(const std::string& str, char delimiter);
void parseAliasPattern(const char* input, std::string& name, char*& command);

std::vector<std::string> split(const std::string& str, char delimiter);
void parseAliasPattern(const char* input, std::string& name, char*& command);
std::string readFile(const std::string& path);

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line): Command(cmd_line) { }

    virtual ~BuiltInCommand() = default;
};

class ExternalCommand : public Command {
    bool isBackground;
    bool isComplex;
public:
    ExternalCommand(const char *cmd_line);

    virtual ~ExternalCommand() {
        delete[] this->cmd;
    }

    void execute() override;
};

class ChangePromptCommand : public BuiltInCommand {
    // TODO: Add your data members public:
    std::string newPrompt;
public:
    explicit ChangePromptCommand(const char *cmd_line);

    virtual ~ChangePromptCommand() = default;

    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
public:
    explicit RedirectionCommand(const char *cmd_line);

    virtual ~RedirectionCommand() {}

    void execute() override;
};

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const char *cmd_line);

    virtual ~PipeCommand() {
    }

    void execute() override;
};

class DiskUsageCommand : public Command {
public:
    DiskUsageCommand(const char *cmd_line);

    virtual ~DiskUsageCommand() {
    }

    void execute() override;
};

class WhoAmICommand : public Command {
public:
    WhoAmICommand(const char *cmd_line);

    virtual ~WhoAmICommand() {
    }

    void execute() override;
};

class NetInfo : public Command {
    // TODO: Add your data members **BONUS: 10 Points**
public:
    NetInfo(const char *cmd_line);

    virtual ~NetInfo() {
    }

    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
    // TODO: Add your data members public:
public:
    ChangeDirCommand(const char *cmd_line, char **plastPwd);

    virtual ~ChangeDirCommand() {
    }

    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char *cmd_line): BuiltInCommand(cmd_line) { }

    virtual ~GetCurrDirCommand() {
    }

    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char *cmd_line): BuiltInCommand(cmd_line) { }

    virtual ~ShowPidCommand() {
    }

    void execute() override;
};

class JobsList {
public:
    class JobEntry {
        // TODO: Add your data members
        int id;
        int pid;
        bool isStopped;
        Command *cmd;
        int signals[RANGEOFSIGNALS];

    public:
        int getId() const {
            return this->id;
        }
        int getPid() const {
            return this->pid;
        }
        bool getIsStopped() {
            return this->isStopped;
        }

        JobEntry(Command *cmd, bool isStopped, int id, int pid)
                : id(id) ,pid(pid), isStopped(isStopped), cmd(cmd) {
            std::fill(std::begin(signals), std::end(signals), 0);
        }

        void setSignal(int num){
            signals[num] = 1;
        }

        ~JobEntry() = default;

        const char* getCmd() const {
            return this->cmd->getOriginalCmdLine();
        }
        void setIsStopped(bool b) {
            this->isStopped = b;
        }
    };

    // TODO: Add your data members
private:
    std::map<int, JobEntry>* jobs;

public:
    JobsList();

    ~JobsList();

    std::map<int, JobEntry>* getJobs() {
        return this->jobs;
    }

    void addJob(Command *cmd, pid_t pid, bool isStopped = false);

    void printJobsList();

    void killAllJobs();

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId);

    void removeJobById(int jobId);

    JobEntry *getLastJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId);

    // TODO: Add extra methods or modify exisitng ones as needed
    void addJob(Command *cmd, bool isStopped, pid_t pid);
};

class QuitCommand : public BuiltInCommand {
    bool kill;
    JobsList* jobs;
public:
    // TODO: Add your data members public:
    QuitCommand(const char *cmd_line, JobsList *jobs);

    virtual ~QuitCommand() = default;

    void execute() override;
};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList* jobs;
public:
    JobsCommand(const char *cmd_line, JobsList *jobs)
            : BuiltInCommand(cmd_line), jobs(jobs) {}

    virtual ~JobsCommand() {
    }

    void execute() override;
};

class KillCommand : public BuiltInCommand {
    int signum;
    unsigned long pid;
    // TODO: Add your data members
public:
    KillCommand(const char *cmd_line, JobsList *jobs);

    virtual ~KillCommand() {
    }

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList* jobs;
    int jobId;
public:
    ForegroundCommand(const char *cmd_line, JobsList *jobs);

    virtual ~ForegroundCommand() = default;

    void execute() override;
};

class AliasCommand : public BuiltInCommand {
    std::string aliasName;
    char* cmdLine;
    bool print;
public:
    AliasCommand(const char *cmd_line);

    virtual ~AliasCommand() {
        if (cmdLine) {
            free(cmdLine);
            cmdLine = nullptr;
        }
    }

    void createAlias(char* cmd);

    void execute() override;
};

class UnAliasCommand : public BuiltInCommand {
    std::vector<std::string > unAlias;
public:
    UnAliasCommand(const char *cmd_line);

    virtual ~UnAliasCommand() {
    }

    void execute() override;
};

class UnSetEnvCommand : public BuiltInCommand {
    std::vector<std::string> envVariables;
public:
    UnSetEnvCommand(const char *cmd_line);

    virtual ~UnSetEnvCommand() = default;

    void execute() override;

    bool doesEnvVarExist(const char *var);
};

class WatchProcCommand : public BuiltInCommand {
    double cpuUsage;
    std::string memoryUsage;
    std::string pidProcess;
    bool isValid;
public:
    WatchProcCommand(const char *cmd_line);

    virtual ~WatchProcCommand() {
    }

    void execute() override;
};

class AliasMap {
private:
    std::vector<std::pair<std::string, char*>> aliases;
    const std::set<std::string> shell_keywords = {
            "chprompt", "showpid", "pwd", "cd", "jobs",
            "fg", "quit", "watchproc", "kill", "alias",
            "unalias", "unsetenv", "du", "whoami", "netinfo"
    };
public:
    AliasMap(AliasMap const &) = delete;
    void operator=(AliasMap const &) = delete;
    AliasMap() = default;
    ~AliasMap(){
        for (auto& pair : aliases) {
            free(pair.second);
        }
    }
    void addAlias(const std::string& name , const char* command);
    int removeAlias(const std::string& name);
    const char* getAlias(const std::string& name) const ;
    void print();
    const std::set<std::string>& getShellKeywords() const {
        return shell_keywords;
    }
};

class SmallShell {
private:
    std::string prompt;
    char* lastDirectory;
    char* currDirectory;
    JobsList* jobs;
    AliasMap* aliases;
    SmallShell();

public:
    Command *CreateCommand(const char *cmd_line);

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() {
        static SmallShell instance; // Guaranteed to be destroyed.

        // Instantiated on first use.

        return instance;
    }

    ~SmallShell();

    void setPrompt(std::string prompt);
    std::string getPrompt();
    void setLastDirectory(char* dir);
    char* getLastDirectory();
    void setcurrDirectory(char* dir);
    char* getcurrDirectory();
    void executeCommand(const char *cmd_line);
    JobsList* getjobs();
    AliasMap* getAliasMap();

    // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
