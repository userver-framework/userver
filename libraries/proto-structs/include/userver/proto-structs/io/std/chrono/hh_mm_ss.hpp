#pragma once

/// @file userver/proto-structs/io/std/chrono/hh_mm_ss.hpp
/// @brief Provides `std::chrono::hh_mm_ss` proto struct field support.

#include <chrono>

#include <userver/proto-structs/type_mapping.hpp>

namespace google::type {
class TimeOfDay;
}  // namespace google::type

USERVER_NAMESPACE_BEGIN

namespace proto_structs::traits {

template <typename TDuration>
struct CompatibleMessageTrait<std::chrono::hh_mm_ss<TDuration>> {
    using Type = ::google::type::TimeOfDay;
};

}  // namespace proto_structs::traits

USERVER_NAMESPACE_END
