#pragma once

#include "seer/Log.h"
#include <sml.hpp>
#include <boost/algorithm/string.hpp>
#include <sstream>

namespace gui::sm {

using namespace boost::sml;

struct Logger {
    template <class SM, class TEvent>
    void log_process_event(const TEvent&) {
        seer::log_infof(
            "[%s][process_event] %s", aux::get_type_name<SM>(), aux::get_type_name<TEvent>());
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
        seer::log_infof(
            "[%s][transition] %s -> %s", aux::get_type_name<SM>(), src.c_str(), dst.c_str());
    }
};

template <class T>
void dump_transition(std::ostream& ss) noexcept {
    auto src_state = std::string{boost::sml::aux::string<typename T::src_state>{}.c_str()};
    auto dst_state = std::string{boost::sml::aux::string<typename T::dst_state>{}.c_str()};
    if (dst_state == "X") {
        dst_state = "[*]";
    }

    if (T::initial) {
        ss << "[*] --> " << src_state << std::endl;
    }

    ss << src_state << " --> " << dst_state;

    const auto has_event = !boost::sml::aux::is_same<typename T::event, boost::sml::anonymous>::value;
    const auto has_guard = !boost::sml::aux::is_same<typename T::guard, boost::sml::front::always>::value;
    const auto has_action = !boost::sml::aux::is_same<typename T::action, boost::sml::front::none>::value;

    if (has_event || has_guard || has_action) {
        ss << " :";
    }

    if (has_event) {
        ss << " " << boost::sml::aux::get_type_name<typename T::event>();
    }

    if (has_guard) {
        ss << " [" << boost::sml::aux::get_type_name<typename T::guard::type>() << "]";
    }

    if (has_action) {
        ss << " / " << boost::sml::aux::get_type_name<typename T::action::type>();
    }

    ss << std::endl;
}

template <template <class...> class T, class... Ts>
void dump_transitions(const T<Ts...>&, std::ostream& ss) noexcept {
    int _[]{0, (dump_transition<Ts>(ss), 0)...};
    (void)_;
}

template <class SM>
std::string dump(const SM&) noexcept {
    std::stringstream ss;
    ss << "@startuml" << std::endl << std::endl;
    dump_transitions(typename SM::transitions{}, ss);
    ss << std::endl << "@enduml" << std::endl;
    auto result = ss.str();
    boost::algorithm::replace_all(result, "gui::sm::", "");
    boost::algorithm::replace_all(result, "StateMachine::a_", "");
    return result;
}

} // namespace gui::sm
