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
bool discord_window_open = false;

inline int no_out_system(const std::string& command) {
    return system((command + " > /dev/null").c_str());
}

void atexit_handler() {
    if (xdo != NULL) {
        std::cout << "Sending SIGCONT to all Discord processes\n";
        no_out_system("killall -18 Discord");
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
    signal(SIGQUIT, signal_handler);

    for (;; WAIT) {
        if (no_out_system("pidof -s Discord") == 0) {
            no_out_system("renice -n 20 --pid `pidof Discord`");

            // Check if Discord window is open
            if (no_out_system("bash -c 'wmctrl -l | grep -o \"$HOSTNAME.*\" | sed \"s/$HOSTNAME //g\" | grep Discord'") == 0) {
                if (!discord_window_open) {
                    std::cout << "Discord window opened\n";
                    discord_window_open = true;
                }
            } else {
                if (discord_window_open) {
                    std::cout << "Discord window closed\n";
                    discord_window_open = false;
                    std::cout << "Sending SIGCONT to all Discord processes\n";
                    no_out_system("killall -18 Discord");
                    discord_stopped = false;
                }
                continue;
            }

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
                    no_out_system("killall -19 Discord");
                    discord_stopped = true;
                }
            } else if (discord_stopped) {
                std::cout << "Sending SIGCONT to all Discord processes\n";
                no_out_system("killall -18 Discord");
                discord_stopped = false;
            }
        } else {
            discord_stopped = false;
            discord_window_open = false;
        }
    }

    return 0;
}
