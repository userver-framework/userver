#pragma once

#include "any.hpp"

#include <userver/chaotic/additional_properties.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>
#include <userver/chaotic/sax_parser.hpp>

namespace ns {

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_WithAnyFieldFieldNamepayload = "payload";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::WithAnyField, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::WithAnyField, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::formats::json::Value>, &::ns::WithAnyField::payload, k_ns_WithAnyFieldFieldNamepayload>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<WithAnyField>);

}  // namespace ns

