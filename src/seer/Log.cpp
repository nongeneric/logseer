#include "Log.h"
#include "version.h"

#ifdef WIN32
#include "windows.h"
#endif
#include <spdlog/version.h>

#if SPDLOG_VERSION > 10200
#define SPDLOG_FMT_EXTERNAL
#endif

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <filesystem>
#include <chrono>
#include <sstream>
#include <string>
#include <iomanip>

namespace {

std::shared_ptr<spdlog::logger> g_logger;
bool g_enabled = false;

std::string getLogPath() {
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::ostringstream ss;
    ss << "logseer_" << std::put_time(std::localtime(&now), "%m-%d.%H-%M-%S") << ".log";
    return (std::filesystem::temp_directory_path() / ss.str()).string();
}

void log_init(bool file) {
    if (g_logger)
        return;

    std::vector<spdlog::sink_ptr> sinks {
        std::make_shared<spdlog::sinks::stdout_sink_mt>()
    };

    std::string logFilePath;
    if (file) {
        logFilePath = getLogPath();
        sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, true));
    }

    g_logger = std::make_shared<spdlog::logger>("name", begin(sinks), end(sinks));
    spdlog::register_logger(g_logger);
    spdlog::set_pattern("%v");
    seer::log_infof("{} {}{}", g_name, g_version, file ? fmt::format(" (file {})", logFilePath) : "");
    spdlog::set_pattern("%H:%M:%S.%f [%t] %v");
    spdlog::flush_every(std::chrono::seconds(3));
}

} // namespace

void seer::log_info(const char* message) {
    if (!g_enabled)
        return;
    g_logger->info(message);
}

void seer::log_enable(bool file) {
    g_enabled = true;
#ifdef WIN32
    if (AttachConsole(ATTACH_PARENT_PROCESS))
    {
        freopen("CON", "w", stdout);
        freopen("CON", "w", stderr);
    }
#endif
    log_init(file);
}
