#pragma once

/// @file userver/utils/null_terminated_view.hpp
/// @brief @copybrief utils::NullTerminatedView
/// @ingroup userver_universal

#include <string>
#include <string_view>
#include <type_traits>

#include <fmt/core.h>

#include <userver/formats/serialize/to.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @ingroup userver_containers
///
/// @brief Non-empty string view to a null terminated char array.
class NullTerminatedView : public std::string_view {
public:
    NullTerminatedView() = delete;

    constexpr NullTerminatedView(const char* str) noexcept : std::string_view{str} {
        // data()[size()] == '\0' is guaranteed by std::string_view that calls std::strlen(str)
    }

    NullTerminatedView(const std::string& str) noexcept : std::string_view{str} {}

    constexpr const char* c_str() const noexcept { return data(); }

    /// Constructs a NullTerminatedView from a pointer and size.
    /// @warning `str[len]` should be '\0'.
    static constexpr NullTerminatedView UnsafeMake(const char* str, std::size_t len) noexcept {
        return NullTerminatedView{str, len};
    }

private:
    constexpr NullTerminatedView(const char* str, std::size_t len) noexcept : std::string_view{str, len} {
        UASSERT_MSG(str, "null not allowed");
        UASSERT_MSG(str[len] == '\0', "Not null terminated");
    }
};

template <class Value>
Value Serialize(NullTerminatedView view, formats::serialize::To<Value>) {
    return typename Value::Builder(std::string_view{view}).ExtractValue();
}

}  // namespace utils

USERVER_NAMESPACE_END

template <>
struct fmt::formatter<USERVER_NAMESPACE::utils::NullTerminatedView, char> : fmt::formatter<std::string_view> {};
