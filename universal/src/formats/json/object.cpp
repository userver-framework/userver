#include <userver/formats/json/object.hpp>

#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {

Object::Object()
    : Value(ValueBuilder{common::Type::kObject}.ExtractValue())
{}

Object::Object(ValueBuilder&& builder)
    : Object(std::move(builder).ExtractValue())
{}

}  // namespace formats::json

USERVER_NAMESPACE_END
