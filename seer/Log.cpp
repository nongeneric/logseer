#include "Log.h"

#ifndef __MINGW32__

#include <spdlog/version.h>

#if SPDLOG_VERSION > 10200
#define SPDLOG_FMT_EXTERNAL
#endif

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <boost/filesystem.hpp>
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
    return (boost::filesystem::temp_directory_path() / ss.str()).string();
}

void log_init() {
    if (g_logger)
        return;
    std::vector<spdlog::sink_ptr> sinks = {
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(getLogPath(), true),
        std::make_shared<spdlog::sinks::stdout_sink_mt>()
    };
    g_logger = std::make_shared<spdlog::logger>("name", begin(sinks), end(sinks));
    spdlog::register_logger(g_logger);
    spdlog::set_pattern("%v");
    g_logger->info("logseer log");
    spdlog::set_pattern("%H:%M:%S.%f [%t] %v");
    spdlog::flush_every(std::chrono::seconds(3));
}

} // namespace

void seer::log_info(const char* message) {
    if (!g_enabled)
        return;
    g_logger->info(message);
}

void seer::log_enable() {
    log_init();
    g_enabled = true;
}

#else
#include <windows.h>
#include <iostream>
#include <fstream>

void seer::log_info(const char* message) {
    std::cout << message << std::endl;
}

void seer::log_enable() {
    if (AttachConsole(ATTACH_PARENT_PROCESS))
    {
        freopen("CON", "w", stdout);
        freopen("CON", "w", stderr);
    }
}
#endif
