#pragma once

#include "bformat.h"
#include <string>
#include <type_traits>

namespace seer {

    void log_info(const char* message);

    template <typename... Args>
    void log_infof(const char* message, const Args&... args) {
        log_info(bformat(message, args...).c_str());
    }

} // namespace seer
