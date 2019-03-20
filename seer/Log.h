#pragma once

#include <string>
#include <type_traits>

namespace seer {

    namespace {
        thread_local std::string ssnprintf_buf;

        template <typename T>
        concept bool HasCStr =
            requires(T a) {
                { a.c_str() } -> const char*;
            };

        template <HasCStr T>
        const char* adaptArg(const T& x) {
            return x.c_str();
        }

        template <typename T>
        const T& adaptArg(const T& arg) {
            return arg;
        }
    }

    template <typename... Args>
    std::string ssnprintf(const char* f, const Args&... args) {
        auto len = snprintf(&ssnprintf_buf[0],
                            ssnprintf_buf.size() + 1,
                            f,
                            adaptArg(args)...);
        if (len < 0)
            return "error";
        auto oldLen = ssnprintf_buf.size();
        ssnprintf_buf.resize(len);
        if (static_cast<size_t>(len) > oldLen) {
            snprintf(&ssnprintf_buf[0],
                     ssnprintf_buf.size() + 1,
                     f,
                     adaptArg(args)...);
        }
        return ssnprintf_buf;
    }

    void log_info(const char* message);

    template <typename... Args>
    void log_infof(const char* message, const Args&... args) {
        log_info(ssnprintf(message, args...).c_str());
    }

} // namespace seer
