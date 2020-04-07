#pragma once

#include "bformat.h"
#include <string>
#include <type_traits>

namespace seer {

void log_info(const char* message);
void log_enable(bool value);

template <typename... Args>
void log_infof(const char* message, const Args&... args) {
    log_info(bformat(message, args...).c_str());
}

} // namespace seer
