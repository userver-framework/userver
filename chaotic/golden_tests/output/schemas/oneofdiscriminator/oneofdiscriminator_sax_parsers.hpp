#pragma once

#include "oneofdiscriminator.hpp"

#include <userver/chaotic/additional_properties.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/json/serialize_variant.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>
#include <userver/chaotic/sax_parser.hpp>

namespace ns {

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_AFieldNametype = "type";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_AFieldNamea_prop = "a_prop";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::A, USERVER_NAMESPACE::chaotic::UnknownFields::StoreJson, USERVER_NAMESPACE::chaotic::Field<::ns::A, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>, &::ns::A::type, k_ns_AFieldNametype>, USERVER_NAMESPACE::chaotic::Field<::ns::A, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<int>>, &::ns::A::a_prop, k_ns_AFieldNamea_prop>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<A>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_BFieldNametype = "type";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_BFieldNameb_prop = "b_prop";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::B, USERVER_NAMESPACE::chaotic::UnknownFields::StoreJson, USERVER_NAMESPACE::chaotic::Field<::ns::B, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>, &::ns::B::type, k_ns_BFieldNametype>, USERVER_NAMESPACE::chaotic::Field<::ns::B, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<int>>, &::ns::B::b_prop, k_ns_BFieldNameb_prop>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<B>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_CFieldNameversion = "version";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::C, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::C, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<int>>, &::ns::C::version, k_ns_CFieldNameversion>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<C>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_DFieldNameversion = "version";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::D, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::D, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<int>>, &::ns::D::version, k_ns_DFieldNameversion>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<D>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_IntegerOneOfDiscriminatorFieldNamefoo = "foo";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::IntegerOneOfDiscriminator, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::IntegerOneOfDiscriminator, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::OneOfWithDiscriminator<&::ns::IntegerOneOfDiscriminator::kFoo_Settings, USERVER_NAMESPACE::chaotic::Primitive<::ns::C>, USERVER_NAMESPACE::chaotic::Primitive<::ns::D>>>, &::ns::IntegerOneOfDiscriminator::foo, k_ns_IntegerOneOfDiscriminatorFieldNamefoo>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<IntegerOneOfDiscriminator>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_OneOfDiscriminatorFieldNamefoo = "foo";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::OneOfDiscriminator, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::OneOfDiscriminator, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::OneOfWithDiscriminator<&::ns::OneOfDiscriminator::kFoo_Settings, USERVER_NAMESPACE::chaotic::Primitive<::ns::A>, USERVER_NAMESPACE::chaotic::Primitive<::ns::B>>>, &::ns::OneOfDiscriminator::foo, k_ns_OneOfDiscriminatorFieldNamefoo>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<OneOfDiscriminator>);

}  // namespace ns

