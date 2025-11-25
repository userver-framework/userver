#pragma once

/// @file userver/proto-structs/io/std/chrono/year_month_day.hpp
/// @brief Provides `std::chrono::year_month_day` proto struct field support.

#include <chrono>

#include <userver/proto-structs/type_mapping.hpp>

namespace google::type {
class Date;
}  // namespace google::type

USERVER_NAMESPACE_BEGIN

namespace proto_structs::traits {

template <>
struct CompatibleMessageTrait<std::chrono::year_month_day> {
    using Type = ::google::type::Date;
};

}  // namespace proto_structs::traits

USERVER_NAMESPACE_END
