#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {
    // TODO: Add your implementation
    if (sig_num != SIGINT) {
        return;
    }
    cout << "smash: got ctrl-C" << endl;
    // Check if there is a foreground process
    if (SmallShell::getInstance().getForegroundProcess() != nullptr) {
        // Send SIGINT to the foreground process
        pid_t pid = SmallShell::getInstance().getForegroundProcess()->getPid();
        kill(pid, SIGKILL);
        cout << "smash: process " << pid << " was killed" << endl;
    }
}
