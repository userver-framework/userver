#pragma once

/// @file userver/proto-structs/io/std/chrono/time_point_fwd.hpp
/// @brief Provides `std::chrono::time_point` proto struct field support.

#include <chrono>

#include <userver/proto-structs/type_mapping.hpp>

namespace google::protobuf {
class Timestamp;
}  // namespace google::protobuf

USERVER_NAMESPACE_BEGIN

namespace proto_structs::traits {

template <>
struct CompatibleMessageTrait<std::chrono::time_point<std::chrono::system_clock>> {
    using Type = ::google::protobuf::Timestamp;
};

}  // namespace proto_structs::traits

USERVER_NAMESPACE_END
