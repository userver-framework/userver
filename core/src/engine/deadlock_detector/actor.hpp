#pragma once

#include <fmt/format.h>

#include <userver/utils/string_literal.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::deadlock_detector {

class Actor {
public:
    virtual ~Actor() = default;

    virtual utils::StringLiteral GetActorType() const = 0;
};

inline std::string ToAssertString(const Actor& a) {
    return fmt::format("{} (ptr={})", a.GetActorType(), static_cast<const void*>(&a));
}

}  // namespace engine::deadlock_detector

USERVER_NAMESPACE_END
