#pragma once

#include "enum.hpp"

#include <userver/chaotic/additional_properties.hpp>
#include <userver/chaotic/exception.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/common/meta.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>
#include <userver/chaotic/sax_parser.hpp>

namespace ns {

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_EnumFieldNamefoo = "foo";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::Enum, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::Enum, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<::ns::Enum::Foo>>, &::ns::Enum::foo, k_ns_EnumFieldNamefoo>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<Enum>);

}  // namespace ns

