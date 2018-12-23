#pragma once

#include "seer/Log.h"
#include <sml.hpp>

namespace gui {

    namespace sm {
        using namespace boost::sml;

        struct Logger {
            template <class SM, class TEvent> void log_process_event(const TEvent&) {
                seer::log_infof("[%s][process_event] %s",
                                aux::get_type_name<SM>(),
                                aux::get_type_name<TEvent>());
            }

            template <class SM, class TGuard, class TEvent>
            void log_guard(const TGuard&, const TEvent&, bool result) {
                seer::log_infof("[%s][guard] %s %s %s",
                                aux::get_type_name<SM>(),
                                aux::get_type_name<TGuard>(),
                                aux::get_type_name<TEvent>(),
                                (result ? "[OK]" : "[Reject]"));
            }

            template <class SM, class TAction, class TEvent>
            void log_action(const TAction&, const TEvent&) {
                seer::log_infof("[%s][action] %s %s",
                                aux::get_type_name<SM>(),
                                aux::get_type_name<TAction>(),
                                aux::get_type_name<TEvent>());
            }

            template <class SM, class TSrcState, class TDstState>
            void log_state_change(const TSrcState& src, const TDstState& dst) {
                seer::log_infof("[%s][transition] %s -> %s",
                                aux::get_type_name<SM>(),
                                src.c_str(),
                                dst.c_str());
            }
        };

    } // namespace sm
} // namespace gui
