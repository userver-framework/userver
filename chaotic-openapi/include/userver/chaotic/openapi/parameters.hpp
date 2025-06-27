#pragma once

#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi {

enum class In {
    kHeader,
    kCookie,
    kPath,
    kQuery,
    kQueryExplode,
};

using Name = const char* const;

template <typename T>
inline constexpr bool kIsTrivialRawType = std::is_integral_v<T> || std::is_enum_v<T>;

template <>
inline constexpr bool kIsTrivialRawType<bool> = true;

template <>
inline constexpr bool kIsTrivialRawType<std::string> = true;

template <>
inline constexpr bool kIsTrivialRawType<double> = true;

template <typename RawType_, typename UserType_>
struct TrivialParameterBase {
    using RawType = RawType_;
    using UserType = UserType_;

    static_assert(kIsTrivialRawType<RawType>);
};

template <In In_, const Name& Name_, typename RawType_, typename UserType_ = RawType_>
struct TrivialParameter {
    static constexpr auto kIn = In_;
    static constexpr auto& kName = Name_;

    using Base = TrivialParameterBase<RawType_, UserType_>;

    static_assert(kIn != In::kQueryExplode);
};

template <In In_, char Delimiter_, typename RawItemType_, typename UserItemType_ = RawItemType_>
struct ArrayParameterBase {
    static constexpr char kDelimiter = Delimiter_;
    static constexpr auto kIn = In_;

    using RawType = std::vector<std::string>;
    using RawItemType = RawItemType_;
    using UserType = std::vector<UserItemType_>;
    using UserItemType = UserItemType_;
};

template <In In_, const Name& Name_, char Delimiter_, typename RawItemType_, typename UserItemType_ = RawItemType_>
struct ArrayParameter {
    static constexpr auto& kName = Name_;
    static constexpr auto kIn = In_;

    using Base = ArrayParameterBase<In_, Delimiter_, RawItemType_, UserItemType_>;
};

}  // namespace chaotic::openapi

USERVER_NAMESPACE_END
