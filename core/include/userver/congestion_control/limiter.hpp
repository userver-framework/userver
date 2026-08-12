#pragma once

/// @file userver/congestion_control/limiter.hpp
/// @brief @copybrief congestion_control::Limiter

#include <optional>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace congestion_control {

struct Limit {
    std::optional<std::size_t> load_limit;
    std::size_t current_load{0};

    std::string ToLogString() const {
        return "limit=" + (load_limit ? std::to_string(*load_limit) : std::string("(none)"));
    }
};

/// @brief Applies congestion control load limits
class Limiter {
public:
    virtual void SetLimit(const Limit& new_limit) = 0;

protected:
    ~Limiter() = default;
};

}  // namespace congestion_control

USERVER_NAMESPACE_END
