#pragma once

#include "string.hpp"

#include <userver/chaotic/additional_properties.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>
#include <userver/chaotic/sax_parser.hpp>

namespace ns {

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_StringFieldNamefoo = "foo";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::String, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::String, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>, &::ns::String::foo, k_ns_StringFieldNamefoo>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<String>);

}  // namespace ns

