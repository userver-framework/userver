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

template <typename TRawType, typename TUserType>
struct TrivialParameterBase {
    using RawType = TRawType;
    using UserType = TUserType;

    static_assert(kIsTrivialRawType<RawType>);
};

template <In TIn, const Name& Name, typename RawType, typename UserType = RawType>
struct TrivialParameter {
    static constexpr auto kIn = TIn;
    static constexpr auto& kName = Name;

    using Base = TrivialParameterBase<RawType, UserType>;

    static_assert(kIn != In::kQueryExplode);
};

template <In TIn, char Delimiter, typename TRawItemType, typename TUserItemType = TRawItemType>
struct ArrayParameterBase {
    static constexpr char kDelimiter = Delimiter;
    static constexpr auto kIn = TIn;

    using RawType = std::vector<std::string>;
    using RawItemType = TRawItemType;
    using UserType = std::vector<TUserItemType>;
    using UserItemType = TUserItemType;
};

template <In TIn, const Name& Name, char Delimiter, typename TRawItemType, typename TUserItemType = TRawItemType>
struct ArrayParameter {
    static constexpr auto& kName = Name;
    static constexpr auto kIn = TIn;

    using Base = ArrayParameterBase<TIn, Delimiter, TRawItemType, TUserItemType>;
};

}  // namespace chaotic::openapi

USERVER_NAMESPACE_END
