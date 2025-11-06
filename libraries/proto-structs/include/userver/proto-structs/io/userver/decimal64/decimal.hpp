#pragma once

/// @file userver/proto-structs/io/userver/decimal64/decimal64.hpp
/// @brief Provides `userver::decimal64::Decimal` proto struct field support.

#include <userver/decimal64/decimal64.hpp>

#include <userver/proto-structs/type_mapping.hpp>

namespace google::type {
class Decimal;
}  // namespace google::type

USERVER_NAMESPACE_BEGIN

namespace proto_structs::traits {

template <int Prec, typename TRoundPolicy>
struct CompatibleMessageTrait<decimal64::Decimal<Prec, TRoundPolicy>> {
    using Type = ::google::type::Decimal;
};

}  // namespace proto_structs::traits

USERVER_NAMESPACE_END
