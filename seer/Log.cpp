#include "Log.h"

#ifndef __MINGW32__

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>

namespace {
    std::shared_ptr<spdlog::logger> logger;

    void init() {
        if (logger)
            return;
        std::vector<spdlog::sink_ptr> sinks = {
            std::make_shared<spdlog::sinks::stdout_sink_mt>()};
        logger = std::make_shared<spdlog::logger>("name", begin(sinks), end(sinks));
        spdlog::register_logger(logger);
        spdlog::set_pattern("%M:%S.%f %v");
        spdlog::flush_every(std::chrono::seconds(3));
    }

}

void seer::log_info(const char* message) {
    init();
    logger->info(message);
}

#else
#include <iostream>

void seer::log_info(const char* message) {
    std::cout << message << std::endl;
}
#endif
