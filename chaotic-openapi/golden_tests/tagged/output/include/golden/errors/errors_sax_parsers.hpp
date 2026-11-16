#pragma once

#include "golden/errors/errors.hpp"

#include <userver/chaotic/additional_properties.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>
#include <userver/chaotic/sax_parser.hpp>

namespace golden {namespace errors {

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_golden_errors_ErrorFieldNamecode = "code";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_golden_errors_ErrorFieldNamemessage = "message";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::golden::errors::Error, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::golden::errors::Error, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string>>, &::golden::errors::Error::code, k_golden_errors_ErrorFieldNamecode>, USERVER_NAMESPACE::chaotic::Field<::golden::errors::Error, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>, &::golden::errors::Error::message, k_golden_errors_ErrorFieldNamemessage>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<Error>);

}  // namespace errors
}  // namespace golden

