#pragma once

/// @file userver/proto-structs/io/std/chrono/duration.hpp
/// @brief Provides `std::chrono::duration` proto struct field support.

#include <chrono>

#include <userver/proto-structs/type_mapping.hpp>

namespace google::protobuf {
class Duration;
}  // namespace google::protobuf

USERVER_NAMESPACE_BEGIN

namespace proto_structs::traits {

template <typename TRep, typename TPeriod>
struct CompatibleMessageTrait<std::chrono::duration<TRep, TPeriod>> {
    using Type = ::google::protobuf::Duration;
};

}  // namespace proto_structs::traits

USERVER_NAMESPACE_END
