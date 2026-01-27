#pragma once

/// @file userver/formats/json/array.hpp
/// @brief @copybrief formats::json::Array

#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {

class ValueBuilder;

/// @ingroup userver_universal userver_containers userver_formats
///
/// @brief Non-mutable JSON array representation.
///
/// This class is implemented in terms of @ref formats::json::Value and cannot represent anything else but a JSON
/// array. Use it when you need to explicitly state that only JSON array is expected.
class Array final : private Value {
public:
    using Value::const_iterator;
    using Value::const_reverse_iterator;

    /// @brief Creates empty array.
    Array();

    /// @throw TypeMismatchException if @a value is not an array
    explicit Array(const Value& value)
        : Value(value)
    {
        CheckArray();
    }

    /// @throw TypeMismatchException if @a value is not an array
    explicit Array(Value&& value)
        : Value(std::move(value))
    {
        CheckArray();
    }

    /// @brief Creates array extracting value from the @a builder.
    /// @throw TypeMismatchException if extracted value is not an array.
    explicit Array(ValueBuilder&& builder);

    Array(const Array&) = default;
    Array(Array&&) noexcept = default;

    Array& operator=(const Array&) & = default;
    Array& operator=(Array&&) noexcept = default;

    /// @brief Returns @ref formats::json::Value.
    const Value& GetValue() const& { return *this; }

    /// @brief Returns @ref formats::json::Value.
    Value&& ExtractValue() && { return std::move(*this); }

    /// @see @ref formats::json::Value::operator[]
    using Value::operator[];

    // hide overload intended for JSON objects
    Value operator[](std::string_view key) const = delete;

    /// @see @ref formats::json::Value::begin
    using Value::begin;

    /// @see @ref formats::json::Value::end
    using Value::end;

    /// @see @ref formats::json::Value::rbegin
    using Value::rbegin;

    /// @see @ref formats::json::Value::rend
    using Value::rend;

    /// @see @ref formats::json::Value::IsEmpty
    using Value::IsEmpty;

    /// @see @ref formats::json::Value::GetSize
    using Value::GetSize;

    /// @see @ref formats::json::Value::CheckInBounds
    using Value::CheckInBounds;

    /// @see @ref formats::json::Value::As
    using Value::As;

    /// @see @ref formats::json::Value::ConvertTo
    using Value::ConvertTo;

    /// @see @ref formats::json::Value::GetPath
    using Value::GetPath;

    /// @see @ref formats::json::Value::IsRoot
    using Value::IsRoot;

    /// @brief Returns a deep copy of array (see @ref formats::json::Value::Clone).
    Array Clone() const { return Array{GetValue().Clone()}; }
};

/// @brief Compares values.
inline bool operator==(const Array& lhs, const Array& rhs) { return lhs.GetValue() == rhs.GetValue(); }
inline bool operator==(const Array& lhs, const Value& rhs) { return lhs.GetValue() == rhs; }
inline bool operator==(const Value& lhs, const Array& rhs) { return lhs == rhs.GetValue(); }
inline bool operator!=(const Array& lhs, const Array& rhs) { return lhs.GetValue() != rhs.GetValue(); }
inline bool operator!=(const Array& lhs, const Value& rhs) { return lhs.GetValue() != rhs; }
inline bool operator!=(const Value& lhs, const Array& rhs) { return lhs != rhs.GetValue(); }

inline Array Parse(const Value& value, parse::To<Array>) { return Array{value}; }

inline Value Serialize(const Array& array, formats::serialize::To<Value>) { return Value{array.GetValue()}; }

inline Value Serialize(Array&& array, formats::serialize::To<Value>) { return Value{std::move(array).ExtractValue()}; }

}  // namespace formats::json

USERVER_NAMESPACE_END
