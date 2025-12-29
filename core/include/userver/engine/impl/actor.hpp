#pragma once

#include <fmt/format.h>

#include <userver/utils/string_literal.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl::deadlock_detector {

class Actor {
public:
    virtual utils::StringLiteral GetActorType() const = 0;

protected:
    ~Actor() = default;
};

inline std::string ToAssertString(const Actor& a) {
    return fmt::format("{} (ptr={})", a.GetActorType(), static_cast<const void*>(&a));
}

}  // namespace engine::impl::deadlock_detector

USERVER_NAMESPACE_END
