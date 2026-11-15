#pragma once

/// @file userver/chaotic/free_form_object.hpp
/// @brief @copybrief chaotic::FreeFormObject

#include <utility>

#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/formats/common/meta.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/string_builder.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic {

/// @brief A named free-form object of a schema: `type: object` with `additionalProperties: true` only.
///
/// It is a formats::json::Value, and parsing it, DOM and SAX alike, rejects a JSON value which is not an
/// object. A default-constructed one is an empty object. Constructing it from a formats::json::Value does
/// not check the value.
class FreeFormObject final : public formats::json::Value {
public:
    FreeFormObject()
        : formats::json::Value(formats::json::MakeObject())
    {}

    // NOLINTNEXTLINE(google-explicit-constructor)
    FreeFormObject(formats::json::Value value) noexcept
        : formats::json::Value(std::move(value))
    {}
};

template <typename Value>
requires formats::common::IsFormatValue<Value>
FreeFormObject Parse(const Value& value, formats::parse::To<FreeFormObject>) {
    return FreeFormObject{Parse(value, formats::parse::To<Primitive<formats::json::Value, IsObject>>{})};
}

/// @brief Returns `false` if the value is not an object, see @ref formats::parse::MayParseAs.
template <typename Value>
requires formats::common::IsFormatValue<Value>
bool MayParseAs(const Value& value, formats::parse::To<FreeFormObject>) {
    return value.IsObject();
}

template <typename Value>
Value Serialize(const FreeFormObject& value, formats::serialize::To<Value>) {
    return typename Value::Builder{static_cast<const formats::json::Value&>(value)}.ExtractValue();
}

inline void WriteToStream(const FreeFormObject& value, formats::json::StringBuilder& sw) {
    WriteToStream(static_cast<const formats::json::Value&>(value), sw);
}

}  // namespace chaotic

USERVER_NAMESPACE_END
