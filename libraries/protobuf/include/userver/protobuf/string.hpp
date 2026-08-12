#pragma once

/// @file userver/protobuf/string.hpp
/// @brief Operations with Protobuf string type.

#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#if defined(ARCADIA_ROOT)
#include <google/protobuf/port.h>
#endif

USERVER_NAMESPACE_BEGIN

namespace protobuf {

#if defined(ARCADIA_ROOT)
/// @brief String type used by protobuf APIs in Arcadia builds.
using ProtoStringType = TProtoStringType;
#else
/// @brief String type used by protobuf APIs in open-source builds.
using ProtoStringType = std::string;
#endif

namespace impl {

template <typename TStringLike>
concept StringLike = requires(const TStringLike& str) {
    str.data();
    str.size();
};

/// @brief Returns a `std::string_view` over protobuf-compatible string-like data.
template <StringLike TStringLike>
[[nodiscard]] inline std::string_view ToStringView(const TStringLike& str) {
    return {str.data(), str.size()};
}

/// @brief Converts protobuf-compatible string-like data to `std::string`.
template <StringLike TStringLike>
[[nodiscard]] inline decltype(auto) ToString(const TStringLike& str) {
    if constexpr (std::is_same_v<std::decay_t<TStringLike>, std::string>) {
        return str;
    } else {
        return std::string{str.data(), str.size()};
    }
}

/// @brief Preserves an rvalue `std::string` when converting to `std::string`.
[[nodiscard]] inline std::string ToString(std::string&& str) { return std::move(str); }

/// @brief Converts `std::string` to protobuf string type.
/// @note Kept templated so `if constexpr` discards the invalid `std::string`
///       return path when Arcadia protobuf uses `TString` as `ProtoStringType`.
template <typename TProtoString = ProtoStringType>
[[nodiscard]] inline decltype(auto) ToProtoString(const std::string& str) {
    if constexpr (std::is_same_v<TProtoString, std::string>) {
        return str;
    } else {
        return TProtoString{str.data(), str.size()};
    }
}

/// @brief Converts an rvalue `std::string` to protobuf string type.
/// @note Kept templated so the non-selected branch is not instantiated for
///       Arcadia `TString`.
template <typename TProtoString = ProtoStringType>
[[nodiscard]] inline TProtoString ToProtoString(std::string&& str) {
    if constexpr (std::is_same_v<TProtoString, std::string>) {
        return std::move(str);
    } else {
        return TProtoString{std::move(str)};
    }
}

}  // namespace impl

}  // namespace protobuf

USERVER_NAMESPACE_END
