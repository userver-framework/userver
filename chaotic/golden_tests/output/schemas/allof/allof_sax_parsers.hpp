#pragma once

#include "allof.hpp"

#include <userver/chaotic/additional_properties.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/common/merge.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>
#include <userver/chaotic/sax_parser.hpp>

namespace ns {

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_AllOf_Foo__P0FieldNamefoo = "foo";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::AllOf::Foo__P0, USERVER_NAMESPACE::chaotic::UnknownFields::StoreJson, USERVER_NAMESPACE::chaotic::Field<::ns::AllOf::Foo__P0, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>, &::ns::AllOf::Foo__P0::foo, k_ns_AllOf_Foo__P0FieldNamefoo>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<AllOf::Foo__P0>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_AllOf_Foo__P1FieldNamebar = "bar";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::AllOf::Foo__P1, USERVER_NAMESPACE::chaotic::UnknownFields::StoreJson, USERVER_NAMESPACE::chaotic::Field<::ns::AllOf::Foo__P1, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<int>>, &::ns::AllOf::Foo__P1::bar, k_ns_AllOf_Foo__P1FieldNamebar>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<AllOf::Foo__P1>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::impl::JsonDomParser<AllOf::Foo>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<AllOf::Foo>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_AllOfFieldNamefoo = "foo";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::AllOf, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::AllOf, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<::ns::AllOf::Foo>>, &::ns::AllOf::foo, k_ns_AllOfFieldNamefoo>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<AllOf>);

}  // namespace ns

