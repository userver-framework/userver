#pragma once

#include <stdexcept>
#include <string>

#include <userver/utils/regex.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic {

template <const std::string_view& Regex>
struct Pattern final {
    static const utils::regex kRegex;

    template <typename ErrorReporter>
    static void Validate(std::string_view value, ErrorReporter report_error) {
        if (!utils::regex_search(value, kRegex)) {
            report_error("doesn't match regex");
        }
    }
};

template <const std::string_view& Regex>
inline const utils::regex Pattern<Regex>::kRegex{Regex};

}  // namespace chaotic

USERVER_NAMESPACE_END
