#pragma once

/// @file userver/proto-structs/io/userver/utils/box.hpp
/// @brief Provides `userver::utils::Box` proto struct field support.

#include <userver/utils/box.hpp>

#include <userver/proto-structs/type_mapping.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::traits {

template <typename T>
struct CompatibleMessageTrait<utils::Box<T>> {
    using Type = CompatibleMessageType<T>;
};

}  // namespace proto_structs::traits

USERVER_NAMESPACE_END
