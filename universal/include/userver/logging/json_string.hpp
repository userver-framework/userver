#pragma once

/// @file userver/logging/json_string.hpp
/// @brief @copybrief logging::JsonString

#include <string>
#include <string_view>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

/// One line json string.
/// JSON log formats write such tags directly as a sub-JSON, without packing them in a string.
class JsonString {
public:
    /// @brief Constructs from provided json object.
    /// The generated json string is an one line string.
    /*implicit*/ JsonString(const formats::json::Value& value);

    /// @brief Constructs from provided json string. It is the user's
    /// responsibility to ensure that the input json string is valid.
    /// New lines will be removed during construction.
    explicit JsonString(std::string json) noexcept;

    /// @brief Constructs null json (see GetValue for details)
    JsonString() noexcept = default;

    JsonString(JsonString&&) noexcept = default;
    JsonString(const JsonString&) = default;

    JsonString& operator=(JsonString&&) noexcept = default;
    JsonString& operator=(const JsonString&) = default;

    /// @brief Returns view to json
    std::string_view GetView() const noexcept;

private:
    std::string json_;
};

void WriteToStream(const JsonString& value, formats::json::StringBuilder& sw);

}  // namespace logging

USERVER_NAMESPACE_END
