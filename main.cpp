#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <algorithm>
#include <assert.h>
#include <chrono>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sched.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#define INTERVAL 250
#define WAIT     std::this_thread::sleep_for(std::chrono::milliseconds(INTERVAL))

#define MAX_PROPERTY_VALUE_LEN 4096

std::string process_name;

int cached_kill(pid_t pid, int send) {
    static std::unordered_map<pid_t, int> state;
    if (state.find(pid) == state.end()) {
        state[pid] = SIGCONT;
    }

    if (state[pid] != send) {
        state[pid] = send;
        return kill(pid, send);
    } else {
        return 0;
    }
}

bool is_file(const char* path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

bool is_number(const char* s) {
    for (size_t i = 0; s[i] != '\0'; i++)
        if (!isdigit(s[i]))
            return false;

    return true;
}

inline std::string slurp(const std::string& path) {
    std::ifstream file(path);
    if (file.is_open()) {
        std::cerr << "Error slurping file \"" << path << "\": " << strerror(errno) << std::endl;
        return std::string(std::istreambuf_iterator<char> {file}, {});
    } else {
        return std::string();
    }
}

unsigned char* get_property(Display* dpy, Window window, Atom prop_type, Atom prop, unsigned long* size) {
    Atom type;
    int format;
    unsigned long bytes_after;
    unsigned char* ret;

    XGetWindowProperty(
        dpy,
        window,
        prop,
        0,
        MAX_PROPERTY_VALUE_LEN / 4,
        False,
        prop_type,
        &type,
        &format,
        size,
        &bytes_after,
        &ret);

    assert(type == prop_type);

    return ret;
}

std::vector<std::string> get_procfs_folders(bool include_this_process = false) {
    std::vector<std::string> procfs_folders;

    DIR* procdir = opendir("/proc");
    struct dirent* entry;
    while ((entry = readdir(procdir))) {
        char full_path[7 + strlen(entry->d_name)];
        memset(full_path, 0, sizeof(full_path));
        strcat(full_path, "/proc/");
        strcat(full_path, entry->d_name);

        if (!is_file(full_path) && is_number(entry->d_name)) {
            if (!include_this_process) {
                if (getpid() != atoi(entry->d_name)) {
                    procfs_folders.push_back(entry->d_name);
                }
            } else {
                procfs_folders.push_back(entry->d_name);
            }
        }
    }
    closedir(procdir);

    return procfs_folders;
}

pid_t get_window_pid(Display* dpy, Window window) {
    unsigned long pid_size;
    unsigned long* pid = (unsigned long*) get_property(dpy, window, XA_CARDINAL, XInternAtom(dpy, "_NET_WM_PID", False), &pid_size);

    pid_t ret = *pid;
    XFree(pid);
    return ret;
}

void atexit_handler() {
    std::vector<std::string> procfs_folders = get_procfs_folders();

    for (const auto& folder : procfs_folders) {
        std::string name = slurp("/proc/" + folder + "/comm");
        if (name == process_name) {
            kill(stoi(folder), SIGCONT);
        }
    }
}

inline void signal_handler(int signal_num) {
    exit(signal_num);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " PROG_NAME" << std::endl;
        std::cerr << "Error: Missing " << 2 - argc << " argument(s)" << std::endl;
        return 1;
    }

    process_name = argv[1];
    process_name.push_back('\n');

    Display* dpy;
    if ((dpy = XOpenDisplay(nullptr)) == nullptr) {
        std::cerr << "Error: Failed to open X display" << std::endl;
        return 1;
    }
    Window root = DefaultRootWindow(dpy);

    Atom client_list_atom = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
    Atom active_window_atom = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);

    atexit(atexit_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGHUP, signal_handler);
    signal(SIGQUIT, signal_handler);

    for (;; WAIT) {
        std::vector<std::string> procfs_folders = get_procfs_folders();

        std::vector<pid_t> processes;
        for (const auto& folder : procfs_folders) {
            std::string name = slurp("/proc/" + folder + "/comm");
            if (name == process_name) {
                pid_t pid = stoi(folder);
                processes.push_back(pid);

                struct sched_param param {
                    .sched_priority = 0
                };
                setpriority(PRIO_PROCESS, pid, 20);
                sched_setscheduler(stoi(folder), SCHED_IDLE, &param);
            }
        }

        if (!processes.empty()) {
            unsigned long windows_size;
            Window* windows = (Window*) get_property(dpy, root, XA_WINDOW, client_list_atom, &windows_size);

            unsigned long active_window_size;
            Window* active_window = (Window*) get_property(dpy, root, XA_WINDOW, active_window_atom, &active_window_size);

            bool window_open = false;
            for (unsigned long i = 0; i < windows_size; i++) {
                pid_t pid = get_window_pid(dpy, windows[i]);

                if (std::find(processes.begin(), processes.end(), pid) != processes.end()) {
                    window_open = true;
                    break;
                }
            }

            XFree(windows);

            int send;
            if (window_open) {
                if (*active_window != 0) {
                    pid_t pid = get_window_pid(dpy, *active_window);

                    if (std::find(processes.begin(), processes.end(), pid) != processes.end()) {
                        send = SIGCONT;
                    } else {
                        send = SIGSTOP;
                    }
                } else {
                    send = SIGSTOP;
                }
            } else {
                send = SIGCONT;
            }

            XFree(active_window);

            for (const auto& process : processes) {
                cached_kill(process, send);
            }
        }
    }

    XCloseDisplay(dpy);
}