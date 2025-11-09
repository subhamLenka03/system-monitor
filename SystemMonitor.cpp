#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <algorithm>

struct ProcessInfo {
    int pid;
    std::string cmd;
    double cpu;
    double mem;
};

std::vector<int> getAllPIDs() {
    std::vector<int> pids;
    DIR* dir = opendir("/proc");
    if (!dir) return pids;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_DIR) {
            std::string name = entry->d_name;
            if (std::all_of(name.begin(), name.end(), ::isdigit)) {
                pids.push_back(std::stoi(name));
            }
        }
    }
    closedir(dir);
    return pids;
}

ProcessInfo getProcessInfo(int pid) {
    ProcessInfo info;
    info.pid = pid;

    std::ifstream statf("/proc/" + std::to_string(pid) + "/stat");
    std::string statline;
    if (std::getline(statf, statline)) {
        std::istringstream ss(statline);
        std::string temp;
        ss >> temp >> temp >> temp;
        for (int i = 0; i < 10; ++i) ss >> temp; // Skip unused fields
        double utime, stime;
        ss >> utime >> stime;
        info.cpu = (utime + stime) / sysconf(_SC_CLK_TCK);
    } else {
        info.cpu = 0;
    }

    std::ifstream statusf("/proc/" + std::to_string(pid) + "/status");
    std::string line;
    info.mem = 0;
    while (std::getline(statusf, line)) {
        if (line.rfind("VmRSS", 0) == 0) {
            std::istringstream ss(line);
            std::string label;
            double mem_kb;
            ss >> label >> mem_kb;
            info.mem = mem_kb / 1024; // in MB
        }
    }

    std::ifstream cmdf("/proc/" + std::to_string(pid) + "/cmdline");
    if (!std::getline(cmdf, info.cmd)) info.cmd = "";

    return info;
}

void displayProcesses(std::vector<ProcessInfo>& processes) {
    std::cout << "PID\tCPU(s)\tMEM(MB)\tCOMMAND\n";
    for (const auto& p : processes) {
        std::cout << p.pid << "\t" << p.cpu << "\t" << p.mem << "\t" << p.cmd << "\n";
    }
}

void killProcess(int pid) {
    if (kill(pid, SIGKILL) == 0) {
        std::cout << "Killed PID: " << pid << std::endl;
    } else {
        perror("Kill failed");
    }
}

int main() {
    while (true) {
        std::vector<int> pids = getAllPIDs();
        std::vector<ProcessInfo> infos;
        for (int pid : pids) {
            infos.push_back(getProcessInfo(pid));
        }
        // Sort by CPU usage
        std::sort(infos.begin(), infos.end(),
                  [](const ProcessInfo& a, const ProcessInfo& b) { return a.cpu > b.cpu; });
        system("clear");
        displayProcesses(infos);
        std::cout << "Press k<PID> to kill, r to refresh, q to quit: ";
        std::string cmd;
        std::cin >> cmd;
        if (cmd[0] == 'q') break;
        if (cmd[0] == 'k') {
            int pid = std::stoi(cmd.substr(1));
            killProcess(pid);
        }
        // Any other input (like 'r'), just refresh
    }
    return 0;
}
