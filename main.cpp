#include <unistd.h>
extern "C" {
#include <xdo.h>
}
#include <fstream>
#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <string>

#define INTERVAL 0.5
#define WAIT     usleep(INTERVAL * 1e+6)

xdo_t* xdo;
bool discord_stopped = false;

void atexit_handler() {
    if (xdo != NULL) {
        std::cout << "Sending SIGCONT to all Discord processes\n";
        system("killall -18 Discord > /dev/null");
        xdo_free(xdo);
    }
}

inline void signal_handler(int signal_num) {
    exit(signal_num);
}

int main() {
    if ((xdo = xdo_new(NULL)) == NULL) {
        std::cerr << "Failed to create xdo instance\n";
        return 1;
    }

    atexit(atexit_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGHUP, signal_handler);

    for (;; WAIT) {
        if (system("pidof -s Discord > /dev/null") == 0) {
            system("renice -n 20 --pid `pidof Discord` > /dev/null");

            Window active;
            xdo_get_active_window(xdo, &active);
            if (active == 0) {
                continue;
            }
            int pid = xdo_get_pid_window(xdo, active);
            if (pid == 0) {
                continue;
            }

            std::ifstream comm_file("/proc/" + std::to_string(pid) + "/comm");
            std::string comm((std::istreambuf_iterator<char>(comm_file)),
                std::istreambuf_iterator<char>());
            comm_file.close();
            comm.erase(comm.size() - 1);

            if (comm != "Discord") {
                if (!discord_stopped) {
                    std::cout << "Sending SIGSTOP to all Discord processes\n";
                    system("killall -19 Discord > /dev/null");
                    discord_stopped = true;
                }
            } else if (discord_stopped) {
                std::cout << "Sending SIGCONT to all Discord processes\n";
                system("killall -18 Discord > /dev/null");
                discord_stopped = false;
            }
        } else if (discord_stopped) {
            discord_stopped = false;
        }
    }

    return 0;
}
