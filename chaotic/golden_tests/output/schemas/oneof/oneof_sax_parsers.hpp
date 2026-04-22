#pragma once

#include <userver/chaotic/object.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/sax_parser.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/variant.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

#include "oneof.hpp"
#include "oneof_parsers.ipp"

namespace ns {

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::OneOf, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::OneOf,
        USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Variant<
            USERVER_NAMESPACE::chaotic::Primitive<int>, USERVER_NAMESPACE::chaotic::Primitive<std::string>>>,
        &::ns::OneOf::foo, ::ns::OneOf::kFieldNamefoo>>> ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::OneOf>);

}  // namespace ns

