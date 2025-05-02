// Ver: 10-4-2025
#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <map>

#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define DEFAULT_PROMPT "smash"
#define NUMBEROFSIGNALS 3
#define RANGEOFSIGNALS 32

std::vector<std::string> split(const std::string& str, char delimiter);

class Command {
    // TODO: Add your data members
    const char *cmd;
public:
    Command(const char *cmd_line): cmd(cmd_line) { }

    virtual ~Command() = default;

    virtual void execute() = 0;

    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed

    const char* getCmd() {
        return this->cmd;
    }
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line): Command(cmd_line) { }

    virtual ~BuiltInCommand() = default;
};

class ExternalCommand : public Command {
public:
    ExternalCommand(const char *cmd_line);

    virtual ~ExternalCommand() = default;

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
    GetCurrDirCommand(const char *cmd_line);

    virtual ~GetCurrDirCommand() {
    }

    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char *cmd_line);

    virtual ~ShowPidCommand() {
    }

    void execute() override;
};

class JobsList;

class QuitCommand : public BuiltInCommand {
    // TODO: Add your data members public:
    QuitCommand(const char *cmd_line, JobsList *jobs);

    virtual ~QuitCommand() {
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
        const char* getCmd() const {
            return this->cmd->getCmd();
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

        ~JobEntry() {
            delete cmd;
        }

    };

    // TODO: Add your data members
private:
    std::map<int, JobEntry>* jobs;

public:
    JobsList();

    ~JobsList();

    void addJob(Command *cmd, bool isStopped = false);

    void printJobsList();

    void killAllJobs();

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId);

    void removeJobById(int jobId);

    JobEntry *getLastJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId);

    // TODO: Add extra methods or modify exisitng ones as needed
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
public:
    ForegroundCommand(const char *cmd_line, JobsList *jobs);

    virtual ~ForegroundCommand() {
    }

    void execute() override;
};

class AliasCommand : public BuiltInCommand {
public:
    AliasCommand(const char *cmd_line);

    virtual ~AliasCommand() {
    }

    void execute() override;
};

class UnAliasCommand : public BuiltInCommand {
public:
    UnAliasCommand(const char *cmd_line);

    virtual ~UnAliasCommand() {
    }

    void execute() override;
};

class UnSetEnvCommand : public BuiltInCommand {
public:
    UnSetEnvCommand(const char *cmd_line);

    virtual ~UnSetEnvCommand() {
    }

    void execute() override;
};

class WatchProcCommand : public BuiltInCommand {
public:
    WatchProcCommand(const char *cmd_line);

    virtual ~WatchProcCommand() {
    }

    void execute() override;
};

class SmallShell {
private:
    std::string prompt;
    char* lastDirectory;
    char* currDirectory;
    JobsList* jobs;
    SmallShell();

public:
    Command *CreateCommand(const char *cmd_line);

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
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

    // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
