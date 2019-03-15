#pragma once

#include <string>
#include <type_traits>

namespace seer {

    namespace {
        thread_local std::string ssnprintf_buf;

        template <typename T>
        decltype(auto) adaptArg(T&& arg) {
            using baseType = std::remove_reference_t<std::remove_cv_t<T>>;
            if constexpr (std::is_same<baseType, std::string>::value) {
                return arg.c_str();
            } else {
                return std::forward<T>(arg);
            }
        }
    }

    template <typename... Args>
    std::string ssnprintf(const char* f, Args&&... args) {
        auto len = snprintf(&ssnprintf_buf[0],
                            ssnprintf_buf.size() + 1,
                            f,
                            adaptArg(std::forward<Args>(args))...);
        if (len < 0)
            return "error";
        auto oldLen = ssnprintf_buf.size();
        ssnprintf_buf.resize(len);
        if (static_cast<size_t>(len) > oldLen) {
            snprintf(&ssnprintf_buf[0],
                     ssnprintf_buf.size() + 1,
                     f,
                     adaptArg(std::forward<Args>(args))...);
        }
        return ssnprintf_buf;
    }

    void log_info(const char* message);

    template <typename... Args>
    void log_infof(const char* message, Args&&... args) {
        log_info(ssnprintf(message, std::forward<Args>(args)...).c_str());
    }

} // namespace seer
