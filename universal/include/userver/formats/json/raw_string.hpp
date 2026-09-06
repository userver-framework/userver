#pragma once

/// @file userver/formats/json/raw_string.hpp
/// @brief @copybrief formats::json::RawString

#include <string>
#include <string_view>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {

/// @brief JSON fragment stored as text and written to output without re-encoding.
class RawString {
public:
    /// @brief Constructs from provided json object.
    /*implicit*/ RawString(const Value& value);

    /// @brief Constructs from provided json string. It is the user's
    /// responsibility to ensure that the input json string is valid.
    explicit RawString(std::string json) noexcept;

    /// @brief Constructs empty json object (`{}`).
    RawString() noexcept = default;

    RawString(RawString&&) noexcept = default;
    RawString(const RawString&) = default;

    RawString& operator=(RawString&&) noexcept = default;
    RawString& operator=(const RawString&) = default;

    /// @brief Returns view to json
    std::string_view GetView() const noexcept;

    friend bool operator==(const RawString& lhs, const RawString& rhs) noexcept {
        return lhs.GetView() == rhs.GetView();
    }

private:
    std::string json_;
};

void WriteToStream(const RawString& value, StringBuilder& sw);

Value Serialize(const RawString& json, formats::serialize::To<Value>);

RawString Parse(const Value& value, formats::parse::To<RawString>);

}  // namespace formats::json

USERVER_NAMESPACE_END
