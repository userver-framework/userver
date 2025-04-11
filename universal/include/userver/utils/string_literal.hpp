#pragma once

/// @file userver/utils/string_literal.hpp
/// @brief @copybrief utils::StringLiteral
/// @ingroup userver_universal

#include <string>
#include <string_view>
#include <type_traits>

#include <fmt/core.h>

#include <userver/compiler/impl/constexpr.hpp>
#include <userver/formats/serialize/to.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/null_terminated_view.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @ingroup userver_containers
///
/// @brief Non-empty string view to a compile time known null terminated char array that lives for the lifetime of
/// program, as per [lex.string]; a drop-in replacement for `static const std::string kVar = "value"` and
/// `constexpr std::string_view kVar = "value"`.
class StringLiteral : public NullTerminatedView {
public:
    StringLiteral() = delete;

#if defined(__clang__) && __clang_major__ < 18
    // clang-16 and below lose (optimize out) the pointer to `literal` with consteval. Clang-18 is know to work
    constexpr
#else
    USERVER_IMPL_CONSTEVAL
#endif
        StringLiteral(const char* literal) noexcept
        : NullTerminatedView{literal} {
        // data()[size()] == '\0' is guaranteed by std::string_view that calls std::strlen(literal)
    }

    /// Constructs a StringLiteral from a pointer and size.
    /// @warning `str[len]` should be '\0' and `str` should point to compile time literal.
    static constexpr StringLiteral UnsafeMake(const char* str, std::size_t len) noexcept {
        return StringLiteral(str, len);
    }

private:
    explicit constexpr StringLiteral(const char* str, std::size_t len) noexcept
        : NullTerminatedView{NullTerminatedView::UnsafeMake(str, len)} {}
};

template <class Value>
Value Serialize(StringLiteral literal, formats::serialize::To<Value>) {
    return typename Value::Builder(std::string_view{literal}).ExtractValue();
}

}  // namespace utils

USERVER_NAMESPACE_END

template <>
struct fmt::formatter<USERVER_NAMESPACE::utils::StringLiteral, char> : fmt::formatter<std::string_view> {};
