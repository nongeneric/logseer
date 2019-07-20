#include "Log.h"

#ifndef __MINGW32__

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

    void init() {
        if (g_logger)
            return;
        std::vector<spdlog::sink_ptr> sinks = {
            std::make_shared<spdlog::sinks::basic_file_sink_mt>(getLogPath(), true)
        };
        g_logger = std::make_shared<spdlog::logger>("name", begin(sinks), end(sinks));
        spdlog::register_logger(g_logger);
        spdlog::set_pattern("%v");
        g_logger->info("logseer log");
        spdlog::set_pattern("%H:%M:%S.%f [%t] %v");
        spdlog::flush_every(std::chrono::seconds(3));
    }

}

void seer::log_info(const char* message) {
    if (!g_enabled)
        return;
    init();
    g_logger->info(message);
}

void seer::log_enable(bool value) {
    g_enabled = value;
}

#else
#include <iostream>

void seer::log_info(const char* message) {
    std::cout << message << std::endl;
}

void seer::log_enable(bool) { }
#endif
