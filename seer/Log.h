#pragma once

#include <string>

namespace seer {

    namespace {
        thread_local std::string ssnprintf_buf;
    }

    template <typename... Args>
    std::string ssnprintf(const char* f, Args... args) {
        auto len = snprintf(0, 0, f, args...);
        ssnprintf_buf.resize(len);
        len = snprintf(&ssnprintf_buf[0], len + 1, f, args...);
        return ssnprintf_buf;
    }

    void log_info(const char* message);

    template <typename... Args>
    void log_infof(const char* message, Args... args) {
        log_info(ssnprintf(message, args...).c_str());
    }

} // namespace seer
