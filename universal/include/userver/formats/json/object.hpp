#pragma once

/// @file userver/formats/json/object.hpp
/// @brief @copybrief formats::json::Object

#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {

/// @ingroup userver_universal userver_containers userver_formats
///
/// @brief Non-mutable JSON object representation.
///
/// This class is implemented in terms of formats::json::Value and cannot represent anything else but a JSON object.
/// Use it when you need to explicitly state that only JSON object is expected.
class Object final : private Value {
public:
    /// @brief Creates empty object.
    Object();

    /// @throw TypeMismatchException if @a value is not an object
    explicit Object(const Value& value)
        : Value(value)
    {
        CheckObject();
    }

    /// @throw TypeMismatchException if @a value is not an object
    explicit Object(Value&& value)
        : Value(std::move(value))
    {
        CheckObject();
    }

    /// @brief Creates object extracting value from the @a builder.
    /// @throw TypeMismatchException if extracted value is not an object.
    explicit Object(ValueBuilder&& builder);

    Object(const Object&) = default;
    Object(Object&&) noexcept = default;

    Object& operator=(const Object&) & = default;
    Object& operator=(Object&&) noexcept = default;

    /// @brief Returns formats::json::Value.
    const Value& GetValue() const& { return *this; }

    /// @brief Returns formats::json::Value.
    Value&& ExtractValue() && { return std::move(*this); }

    /// @see @ref formats::json::Value::operator[]
    using Value::operator[];

    // hide overload intended for JSON arrays
    Value operator[](std::size_t index) const = delete;

    /// @see @ref formats::json::Value::IsEmpty
    using Value::IsEmpty;

    /// @see @ref formats::json::Value::GetSize
    using Value::GetSize;

    /// @brief Compares values.
    bool operator==(const Object& other) const { return GetValue() == other.GetValue(); }
    bool operator!=(const Object& other) const { return GetValue() != other.GetValue(); }

    /// @see @ref formats::json::Value::As
    using Value::As;

    /// @see @ref formats::json::Value::ConvertTo
    using Value::ConvertTo;

    /// @see @ref formats::json::Value::HasMember
    using Value::HasMember;

    /// @see @ref formats::json::Value::GetPath
    using Value::GetPath;

    /// @see @ref formats::json::Value::IsRoot
    using Value::IsRoot;

    /// @brief Returns a deep copy of object (see @ref formats::json::Value::Clone).
    Object Clone() const { return Object{GetValue().Clone()}; }
};

inline Object Parse(const Value& value, parse::To<Object>) { return Object{value}; }

inline Value Serialize(const Object& object, formats::serialize::To<Value>) { return Value{object.GetValue()}; }

inline Value Serialize(Object&& object, formats::serialize::To<Value>) {
    return Value{std::move(object).ExtractValue()};
}

}  // namespace formats::json

USERVER_NAMESPACE_END
