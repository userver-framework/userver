#pragma once

#include <userver/chaotic/additional_properties.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/sax_parser.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

#include "int.hpp"

namespace ns {

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::Int, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::Int, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<int>>, &::ns::Int::foo,
        ::ns::Int::kFieldNamefoo>>> ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::Int>);

}  // namespace ns

