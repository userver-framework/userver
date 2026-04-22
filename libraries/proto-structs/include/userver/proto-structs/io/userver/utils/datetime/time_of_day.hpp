#pragma once

/// @file userver/proto-structs/io/userver/utils/datetime/time_of_day.hpp
/// @brief Provides `userver::utils::datetime::TimeOfDay` proto struct field support.

#include <userver/utils/time_of_day.hpp>

#include <userver/proto-structs/type_mapping.hpp>

namespace google::type {
class TimeOfDay;
}  // namespace google::type

USERVER_NAMESPACE_BEGIN

namespace proto_structs::traits {

template <typename TDuration>
struct CompatibleMessageTrait<utils::datetime::TimeOfDay<TDuration>> {
    using Type = ::google::type::TimeOfDay;
};

}  // namespace proto_structs::traits

USERVER_NAMESPACE_END
