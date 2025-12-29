#include <userver/formats/json/array.hpp>

#include <userver/formats/json/value_builder.hpp>
#include "userver/formats/common/type.hpp"

USERVER_NAMESPACE_BEGIN

namespace formats::json {

Array::Array()
    : Value(ValueBuilder{common::Type::kArray}.ExtractValue())
{}

Array::Array(ValueBuilder&& builder)
    : Array(std::move(builder).ExtractValue())
{}

}  // namespace formats::json

USERVER_NAMESPACE_END
