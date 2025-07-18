#pragma once

/// @file
/// @brief @copybrief utils::zstring_view
/// @ingroup userver_universal

#include <string>
#include <string_view>
#include <type_traits>

#include <fmt/format.h>

#include <userver/formats/serialize/to.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @ingroup userver_containers
///
/// @brief Non-empty string view type that guarantees null-termination and has a `c_str()` member function.
class zstring_view : public std::string_view {
public:
    zstring_view() = delete;
    zstring_view(const zstring_view& str) = default;

    constexpr zstring_view(const char* str) noexcept : std::string_view{str} {
        // data()[size()] == '\0' is guaranteed by std::string_view that calls std::strlen(str)
    }

    zstring_view(const std::string& str) noexcept : std::string_view{str} {}

    zstring_view& operator=(std::string_view) = delete;
    zstring_view& operator=(const zstring_view&) = default;

    constexpr const char* c_str() const noexcept { return std::string_view::data(); }

    /// Constructs a zstring_view from a pointer and size.
    /// @warning `str[len]` should be '\0'.
    static constexpr zstring_view UnsafeMake(const char* str, std::size_t len) noexcept {
        return zstring_view{str, len};
    }

private:
    constexpr zstring_view(const char* str, std::size_t len) noexcept : std::string_view{str, len} {
        UASSERT_MSG(str, "null not allowed");
        UASSERT_MSG(str[len] == 0, "Not null-terminated");
    }
};

template <class Value>
Value Serialize(zstring_view view, formats::serialize::To<Value>) {
    return typename Value::Builder(std::string_view{view}).ExtractValue();
}

}  // namespace utils

USERVER_NAMESPACE_END

template <>
struct fmt::formatter<USERVER_NAMESPACE::utils::zstring_view, char> : fmt::formatter<std::string_view> {};

namespace fmt {

// Allow fmt::runtime() to work with utils::zstring_view
inline auto runtime(USERVER_NAMESPACE::utils::zstring_view s) -> decltype(fmt::runtime(std::string_view{s})) {
    return fmt::runtime(std::string_view{s});
}

}  // namespace fmt
