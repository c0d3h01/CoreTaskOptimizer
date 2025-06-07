#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <memory>
#include <stdexcept>
#include <regex>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <map>
#include <set>

// ------------- CONFIGURABLE SECTION ------------- //
// All log paths and tunable parameters centralized here for easy edits.

namespace config {
    const std::string LOG_DIR = "/data/adb/modules/task_optimizer/logs/";
    const std::map<std::string, std::string> LOG_FILES = {
        {"launcher", LOG_DIR + "launcher_package.log"},
        {"affinity", LOG_DIR + "affinity.log"},
        {"rt",       LOG_DIR + "rt.log"},
        {"cgroup",   LOG_DIR + "cgroup.log"},
        {"nice",     LOG_DIR + "nice.log"},
        {"ioprio",   LOG_DIR + "ioprio.log"},
        {"irq",      LOG_DIR + "irqaffinity.log"},
        {"main",     LOG_DIR + "task-optimizer-main.log"},
        {"error",    LOG_DIR + "error.log"}
    };

    // Task groups - update here to adjust behaviors
    const std::vector<std::string> TASK_NAMES_HIGH_PRIO = {
        "servicemanag", "zygote", "writeback", "kblockd", "rcu_tasks_kthre", "ufs_clk_gating",
        "mmc_clk_gate", "system", "kverityd", "speedup_resume_wq", "load_tp_fw_wq", "tcm_freq_hop",
        "touch_delta_wq", "tp_async", "wakeup_clk_wq", "thread_fence", "Input"
    };
    const std::vector<std::string> TASK_NAMES_LOW_PRIO = {"ipawq", "iparepwq", "wlan_logging_th"};
    const std::vector<std::string> TASK_NAMES_RT_FF = {
        "kgsl_worker_thread", "devfreq_boost", "mali_jd_thread", "mali_event_thread", "crtc_commit",
        "crtc_event", "pp_event", "rot_commitq_", "rot_doneq_", "rot_fenceq_", "system_server",
        "surfaceflinger", "composer", "fts_wq", "nvt_ts_work"
    };
    const std::vector<std::string> TASK_NAMES_RT_IDLE = {"f2fs_gc"};
    const std::vector<std::string> TASK_NAMES_IO_PRIO = {"f2fs_gc"};

    // CPU masks, IRQ masks, etc. centralized here
    const std::string RENDERTHREAD_MASK = "ff";
    const std::string GPUWORKERS_MASK = "0f";
    const std::string PERFCORE_MASK = "80";
    const std::string MIDCORE_MASK = "70";
    const std::string GENERAL_MASK = "f0";
}

// ------------- LOGGING UTILITY ------------- //
void logMessage(const std::string& message, const std::string& logFile) {
    std::ofstream log(logFile, std::ios::app);
    if (log.is_open()) {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        log << "[" << std::put_time(std::localtime(&now), "%F %T") << "] " << message << std::endl;
    } else {
        std::cerr << "[ERROR] Cannot write to log file: " << logFile << std::endl;
    }
}

// ------------- SHELL EXECUTION UTILITY ------------- //
std::string executeCommand(const std::string& command, const std::string& logKey) {
    std::string result;
    FILE* pipe = popen(command.c_str(), "r");
    if (pipe) {
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        int status = pclose(pipe);
        logMessage("Executed: " + command + (status ? " [fail]" : " [ok]"), config::LOG_FILES.at(logKey));
    } else {
        logMessage("Failed to execute: " + command, config::LOG_FILES.at(logKey));
    }
    return result;
}

// ------------- PROCFS UTILITIES ------------- //
// Directly parse /proc for performance and resilience.

std::vector<pid_t> getProcessIDs(const std::string& namePattern) {
    std::vector<pid_t> pids;
    std::regex pattern(namePattern);
    for (const auto& entry : std::filesystem::directory_iterator("/proc")) {
        if (!entry.is_directory()) continue;
        std::string pidStr = entry.path().filename();
        if (!std::all_of(pidStr.begin(), pidStr.end(), ::isdigit)) continue;
        std::ifstream commFile(entry.path()/"comm");
        if (!commFile) continue;
        std::string comm;
        std::getline(commFile, comm);
        if (std::regex_search(comm, pattern)) {
            pids.push_back(std::stoi(pidStr));
        }
    }
    return pids;
}

std::vector<pid_t> getThreadIDs(pid_t pid, const std::string& threadPattern = "") {
    std::vector<pid_t> tids;
    std::regex pattern(threadPattern);
    std::string taskDir = "/proc/" + std::to_string(pid) + "/task";
    for (const auto& entry : std::filesystem::directory_iterator(taskDir)) {
        if (!entry.is_directory()) continue;
        std::string tidStr = entry.path().filename();
        if (!std::all_of(tidStr.begin(), tidStr.end(), ::isdigit)) continue;
        if (!threadPattern.empty()) {
            std::ifstream commFile(entry.path()/"comm");
            if (!commFile) continue;
            std::string comm;
            std::getline(commFile, comm);
            if (!std::regex_search(comm, pattern)) continue;
        }
        tids.push_back(std::stoi(tidStr));
    }
    return tids;
}

// ------------- CORE OPTIMIZATION ACTIONS ------------- //

void setTaskAffinity(const std::string& namePattern, const std::string& hexMask) {
    for (pid_t pid : getProcessIDs(namePattern)) {
        for (pid_t tid : getThreadIDs(pid)) {
            std::string cmd = "taskset -p " + hexMask + " " + std::to_string(tid);
            executeCommand(cmd, "affinity");
        }
    }
}

void setThreadAffinity(const std::string& namePattern, const std::string& threadPattern, const std::string& hexMask) {
    for (pid_t pid : getProcessIDs(namePattern)) {
        for (pid_t tid : getThreadIDs(pid, threadPattern)) {
            std::string cmd = "taskset -p " + hexMask + " " + std::to_string(tid);
            executeCommand(cmd, "affinity");
        }
    }
}

void setTaskNice(const std::string& namePattern, int niceVal) {
    for (pid_t pid : getProcessIDs(namePattern)) {
        for (pid_t tid : getThreadIDs(pid)) {
            std::string cmd = "renice -n " + std::to_string(niceVal) + " -p " + std::to_string(tid);
            executeCommand(cmd, "nice");
        }
    }
}

void setThreadNice(const std::string& namePattern, const std::string& threadPattern, int niceVal) {
    for (pid_t pid : getProcessIDs(namePattern)) {
        for (pid_t tid : getThreadIDs(pid, threadPattern)) {
            std::string cmd = "renice -n " + std::to_string(niceVal) + " -p " + std::to_string(tid);
            executeCommand(cmd, "nice");
        }
    }
}

void setTaskRt(const std::string& namePattern, int priority) {
    for (pid_t pid : getProcessIDs(namePattern)) {
        for (pid_t tid : getThreadIDs(pid)) {
            std::string cmd = "chrt -p " + std::to_string(priority) + " " + std::to_string(tid);
            executeCommand(cmd, "rt");
        }
    }
}

void setTaskIoPrio(const std::string& namePattern, int classType, int classLevel) {
    for (pid_t pid : getProcessIDs(namePattern)) {
        for (pid_t tid : getThreadIDs(pid)) {
            std::string cmd = "ionice -c " + std::to_string(classType) + " -n " + std::to_string(classLevel) + " -p " + std::to_string(tid);
            executeCommand(cmd, "ioprio");
        }
    }
}

void setIrqAffinity(const std::string& irqPattern, const std::string& hexMask) {
    std::string cmd = "grep -r '" + irqPattern + "' /proc/interrupts | grep -oE '^[0-9]+' | sort -u";
    std::string irqs = executeCommand(cmd, "irq");
    std::istringstream iss(irqs);
    std::string irq;
    while (std::getline(iss, irq)) {
        if (!irq.empty()) {
            std::string setCmd = "echo " + hexMask + " > /proc/irq/" + irq + "/smp_affinity";
            executeCommand(setCmd, "irq");
        }
    }
}

// Pin only the first thread matching pattern to one mask, use another mask for the rest
void pinProcOnFirst(const std::string& taskPattern, const std::string& hexMaskPrime, const std::string& hexMaskOthers) {
    for (pid_t pid : getProcessIDs(taskPattern)) {
        auto tids = getThreadIDs(pid, taskPattern);
        bool first = true;
        for (pid_t tid : tids) {
            std::string mask = first ? hexMaskPrime : hexMaskOthers;
            std::string cmd = "taskset -p " + mask + " " + std::to_string(tid);
            executeCommand(cmd, "affinity");
            first = false;
        }
    }
}

// ------------- MAIN OPTIMIZATION LOGIC ------------- //

void optimizeLauncher(const std::string& launcherPkg) {
    setThreadAffinity(launcherPkg, "RenderThread|GLThread", config::RENDERTHREAD_MASK);
    setThreadAffinity(launcherPkg, "GPU completion|HWC release|hwui|FramePolicy|ScrollPolicy|ged-swd", config::GPUWORKERS_MASK);
}

void optimizeGraphics() {
    setIrqAffinity("msm_drm|fts", config::PERFCORE_MASK);
    setIrqAffinity("kgsl_3d0_irq", config::MIDCORE_MASK);
    setTaskAffinity("surfaceflinger", config::RENDERTHREAD_MASK);
    pinProcOnFirst("crtc_event:", config::PERFCORE_MASK, config::GPUWORKERS_MASK);
    pinProcOnFirst("crtc_commit:", config::PERFCORE_MASK, config::GPUWORKERS_MASK);
    setTaskAffinity("pp_event", config::PERFCORE_MASK);
    setTaskAffinity("mdss_fb|mdss_disp_wake|vsync_retire_work|pq@", config::MIDCORE_MASK);
    setTaskAffinity("fts_wq|nvt_ts_work", config::MIDCORE_MASK);
    setTaskRt("fts_wq|nvt_ts_work", 50);
    setTaskAffinity("hyper@", config::MIDCORE_MASK);
}

void optimizeTasks() {
    for (const auto& t : config::TASK_NAMES_HIGH_PRIO) setTaskNice(t, -15);
    for (const auto& t : config::TASK_NAMES_LOW_PRIO) setTaskNice(t, 0);

    for (const auto& t : config::TASK_NAMES_RT_FF) {
        setTaskNice(t, -15);
        setTaskAffinity(t, config::GENERAL_MASK);
        setTaskRt(t, 50);
    }
    for (const auto& t : config::TASK_NAMES_RT_IDLE) setTaskRt(t, 0);
    for (const auto& t : config::TASK_NAMES_IO_PRIO) setTaskIoPrio(t, 3, 0);
    setTaskNice("thread_fence", -15);

    setTaskRt("kgsl_worker_thread|crtc_commit|crtc_event|pp_event", 16);
    setTaskRt("rot_commitq_|rot_doneq_|rot_fenceq_", 5);
    setTaskRt("system_server|surfaceflinger|composer", 2);
}

// ------------- MAIN ENTRY POINT ------------- //

int main() {
    try {
        std::filesystem::create_directories(config::LOG_DIR);
        logMessage("Core optimization started...", config::LOG_FILES.at("main"));

        // Get launcher package name dynamically
        const std::string INTENT_ACTION = "android.intent.action.MAIN";
        const std::string INTENT_CATEGORY = "android.intent.category.HOME";
        std::string cmd = "pm resolve-activity -a \"" + INTENT_ACTION + "\" -c \"" + INTENT_CATEGORY + "\" | grep packageName | head -1 | cut -d= -f2";
        std::string launcherPkg = executeCommand(cmd, "launcher");
        launcherPkg = launcherPkg.substr(0, launcherPkg.find('\n'));
        if (launcherPkg.empty()) {
            logMessage("Launcher package not found, skipping launcher optimizations.", config::LOG_FILES.at("main"));
        } else {
            optimizeLauncher(launcherPkg);
        }

        optimizeGraphics();
        optimizeTasks();

        logMessage("Task Optimization executed successfully!", config::LOG_FILES.at("main"));
    } catch (const std::exception& e) {
        logMessage(std::string("Exception occurred: ") + e.what(), config::LOG_FILES.at("error"));
        return 1;
    }
    return 0;
}