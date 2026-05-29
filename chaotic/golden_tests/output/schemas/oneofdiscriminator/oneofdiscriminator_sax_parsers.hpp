#pragma once

#include <userver/chaotic/additional_properties.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/sax_parser.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/json/serialize_variant.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

#include "oneofdiscriminator.hpp"

namespace ns {

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::A, USERVER_NAMESPACE::chaotic::UnknownFields::StoreJson,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::A, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>,
        &::ns::A::type, ::ns::A::kFieldNametype>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::A, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<int>>, &::ns::A::a_prop,
        ::ns::A::kFieldNamea_prop>>> ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::A>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::B, USERVER_NAMESPACE::chaotic::UnknownFields::StoreJson,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::B, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>,
        &::ns::B::type, ::ns::B::kFieldNametype>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::B, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<int>>, &::ns::B::b_prop,
        ::ns::B::kFieldNameb_prop>>> ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::B>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::C, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::C, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<int>>, &::ns::C::version,
        ::ns::C::kFieldNameversion>>> ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::C>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::D, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::D, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<int>>, &::ns::D::version,
        ::ns::D::kFieldNameversion>>> ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::D>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::IntegerOneOfDiscriminator, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::IntegerOneOfDiscriminator,
        USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::OneOfWithDiscriminator<
            &::ns::IntegerOneOfDiscriminator::kFoo_Settings, USERVER_NAMESPACE::chaotic::Primitive<::ns::C>,
            USERVER_NAMESPACE::chaotic::Primitive<::ns::D>>>,
        &::ns::IntegerOneOfDiscriminator::foo, ::ns::IntegerOneOfDiscriminator::kFieldNamefoo>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::IntegerOneOfDiscriminator>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::OneOfDiscriminator, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::OneOfDiscriminator,
        USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::OneOfWithDiscriminator<
            &::ns::OneOfDiscriminator::kFoo_Settings, USERVER_NAMESPACE::chaotic::Primitive<::ns::A>,
            USERVER_NAMESPACE::chaotic::Primitive<::ns::B>>>,
        &::ns::OneOfDiscriminator::foo, ::ns::OneOfDiscriminator::kFieldNamefoo>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::OneOfDiscriminator>);

}  // namespace ns

