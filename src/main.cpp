#include <algorithm>
#include <array>
#include <chrono>
#include <dirent.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

namespace config {
    constexpr const char* LOG_DIR = "/data/adb/modules/task_optimizer/logs/";
    constexpr const char* MAIN_LOG = "/data/adb/modules/task_optimizer/logs/main.log";
    constexpr const char* ERROR_LOG = "/data/adb/modules/task_optimizer/logs/error.log";

    // Core task patterns - removed rarely used ones
    constexpr std::array<std::string_view, 8> HIGH_PRIO_TASKS = {
        "servicemanag", "zygote", "system_server", "surfaceflinger", 
        "kblockd", "writeback", "Input", "composer"
    };
    
    constexpr std::array<std::string_view, 6> RT_TASKS = {
        "kgsl_worker_thread", "crtc_commit", "crtc_event", 
        "pp_event", "fts_wq", "nvt_ts_work"
    };
    
    constexpr std::array<std::string_view, 2> LOW_PRIO_TASKS = {
        "f2fs_gc", "wlan_logging_th"
    };

    // CPU masks
    constexpr const char* PERF_MASK = "f0";     // Performance cores
    constexpr const char* EFFICIENT_MASK = "0f"; // Efficiency cores
    constexpr const char* ALL_CORES = "ff";     // All cores
}

// Logger - Will stores logs in MODPATH
// 
class Logger {
public:
    static void log(std::string_view message, bool isError = false) noexcept {
        try {
            const char* logFile = isError ? config::ERROR_LOG : config::MAIN_LOG;
            std::ofstream file(logFile, std::ios::app);
            if (file.is_open()) {
                auto now = std::chrono::system_clock::now();
                auto time = std::chrono::system_clock::to_time_t(now);
                file << "[" << std::put_time(std::localtime(&time), "%H:%M:%S") 
                     << "] " << message << '\n';
            }
        } catch (...) {}
    }
};

// Process utilities
class ProcessUtils {
public:
    static std::vector<pid_t> getProcessIDs(std::string_view pattern) {
        std::vector<pid_t> pids;
        std::regex regex(std::string(pattern), std::regex_constants::optimize);
        
        try {
            std::error_code ec;
            for (const auto& entry : fs::directory_iterator("/proc", ec)) {
                if (ec || !entry.is_directory()) continue;
                
                const auto filename = entry.path().filename().string();
                if (!std::all_of(filename.begin(), filename.end(), ::isdigit)) continue;
                
                std::ifstream commFile(entry.path() / "comm");
                std::string comm;
                if (std::getline(commFile, comm) && std::regex_search(comm, regex)) {
                    pids.push_back(std::stoi(filename));
                }
            }
        } catch (...) {}
        
        return pids;
    }
    
    static std::vector<pid_t> getThreadIDs(pid_t pid) {
        std::vector<pid_t> tids;
        const std::string taskDir = "/proc/" + std::to_string(pid) + "/task";
        
        try {
            std::error_code ec;
            for (const auto& entry : fs::directory_iterator(taskDir, ec)) {
                if (ec || !entry.is_directory()) continue;
                
                const auto tidStr = entry.path().filename().string();
                if (std::all_of(tidStr.begin(), tidStr.end(), ::isdigit)) {
                    tids.push_back(std::stoi(tidStr));
                }
            }
        } catch (...) {}
        
        return tids;
    }
};

// Executor - needed! for exec as shell
std::string executeCmd(std::string_view command) {
    std::string result;
    if (FILE* pipe = popen(std::string(command).c_str(), "r")) {
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe)) {
            result += buffer;
        }
        pclose(pipe);
    }
    return result;
}

// Nice optimizations
class TaskOptimizer {
public:
    static void setAffinity(std::string_view pattern, std::string_view mask) {
        for (pid_t pid : ProcessUtils::getProcessIDs(pattern)) {
            for (pid_t tid : ProcessUtils::getThreadIDs(pid)) {
                executeCmd("taskset -p " + std::string(mask) + " " + std::to_string(tid) + " 2>/dev/null");
            }
        }
    }
    
    static void setNice(std::string_view pattern, int value) {
        for (pid_t pid : ProcessUtils::getProcessIDs(pattern)) {
            for (pid_t tid : ProcessUtils::getThreadIDs(pid)) {
                executeCmd("renice -n " + std::to_string(value) + " -p " + std::to_string(tid) + " 2>/dev/null");
            }
        }
    }
    
    static void setRT(std::string_view pattern, int priority) {
        for (pid_t pid : ProcessUtils::getProcessIDs(pattern)) {
            for (pid_t tid : ProcessUtils::getThreadIDs(pid)) {
                executeCmd("chrt -f -p " + std::to_string(priority) + " " + std::to_string(tid) + " 2>/dev/null");
            }
        }
    }
    
    static void setIOPrio(std::string_view pattern, int ioClass) {
        for (pid_t pid : ProcessUtils::getProcessIDs(pattern)) {
            for (pid_t tid : ProcessUtils::getThreadIDs(pid)) {
                executeCmd("ionice -c " + std::to_string(ioClass) + " -p " + std::to_string(tid) + " 2>/dev/null");
            }
        }
    }
};

void optimizeSystem() {
    Logger::log("[*] Starting system optimization...");
    
    // High priority system tasks
    for (const auto& task : config::HIGH_PRIO_TASKS) {
        TaskOptimizer::setNice(task, -10);
        TaskOptimizer::setAffinity(task, config::PERF_MASK);
    }
    
    // Real-time tasks for graphics/touch
    for (const auto& task : config::RT_TASKS) {
        TaskOptimizer::setRT(task, 50);
        TaskOptimizer::setAffinity(task, config::PERF_MASK);
    }
    
    // Background/low priority tasks
    for (const auto& task : config::LOW_PRIO_TASKS) {
        TaskOptimizer::setNice(task, 5);
        TaskOptimizer::setAffinity(task, config::EFFICIENT_MASK);
        TaskOptimizer::setIOPrio(task, 3); // Idle class
    }
    
    // Optimize launcher if found
    std::string launcher = executeCmd(
        "pm resolve-activity -a android.intent.action.MAIN -c android.intent.category.HOME | "
        "grep packageName | head -1 | cut -d= -f2 | tr -d '\\n'"
    );
    
    if (!launcher.empty()) {
        // Find RenderThread in launcher process
        for (pid_t pid : ProcessUtils::getProcessIDs(launcher)) {
            std::string taskDir = "/proc/" + std::to_string(pid) + "/task";
            std::error_code ec;
            for (const auto& entry : fs::directory_iterator(taskDir, ec)) {
                if (ec) continue;
                std::ifstream commFile(entry.path() / "comm");
                std::string comm;
                if (std::getline(commFile, comm) && comm.find("RenderThread") != std::string::npos) {
                    std::string tid = entry.path().filename();
                    executeCmd("taskset -p " + std::string(config::ALL_CORES) + " " + tid + " 2>/dev/null");
                    executeCmd("chrt -f -p 60 " + tid + " 2>/dev/null");
                }
            }
        }
        Logger::log("Optimized launcher: " + launcher);
    }
    
    // Basic IRQ optimization for touch and display
    std::string touchIrqs = executeCmd("grep -r 'fts\\|nvt\\|synaptics' /proc/interrupts | grep -oE '^[0-9]+' | head -5");
    std::istringstream iss(touchIrqs);
    std::string irq;
    while (std::getline(iss, irq) && !irq.empty()) {
        executeCmd("echo " + std::string(config::PERF_MASK) + " > /proc/irq/" + irq + "/smp_affinity 2>/dev/null");
    }
    
    Logger::log("[*] System optimization completed successfully");
}

int main() {
    try {
        // Create log directory
        std::error_code ec;
        fs::create_directories(config::LOG_DIR, ec);
        
        optimizeSystem();
        return 0;
        
    } catch (const std::exception& e) {
        Logger::log("[!] Critical error: " + std::string(e.what()), true);
        return 1;
    } catch (...) {
        Logger::log("[!] Unknown critical error occurred", true);
        return 1;
    }
}