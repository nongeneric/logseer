#pragma once

#include <fmt/format.h>
#include <string>
#include <type_traits>

namespace seer {

void log_info(const char* message);
void log_enable(bool file);

template <class... Args>
void log_infof(fmt::format_string<Args...> format, Args&&... args) {
    log_info(fmt::format(format, std::forward<Args>(args)...).c_str());
}

} // namespace seer
