#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char *argv[]) {
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    SmallShell &smash = SmallShell::getInstance();
    while (true) {
        try {
            std::cout << smash.getPrompt() << "> ";
            std::string cmd_line;
            std::getline(std::cin, cmd_line);
            smash.executeCommand(cmd_line.c_str());
        } catch (const SmashError &e) {
            std::cerr << "smash error: " << e.what() << std::endl;
        } catch (const SysError &e) {
            perror(("smash error: " + std::string(e.what()) + " failed").c_str());
        }
    }
    return 0;
}