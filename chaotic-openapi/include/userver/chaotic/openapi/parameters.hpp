#pragma once

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <userver/chaotic/convert.hpp>

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

/// @brief Value types of a scalar parameter.
///
/// The parameter is transferred as `RawType`. A `format` of the schema (e.g. `uuid`) gives
/// `FormatType`, and `x-usrv-cpp-type` gives `UserType`. Values are converted
/// `RawType` -> `FormatType` -> `UserType` and back with `Convert` functions, the same way as in bodies.
template <typename TRawType, typename TUserType, typename TFormatType = TRawType>
struct TrivialParameterBase {
    using RawType = TRawType;
    using FormatType = TFormatType;
    using UserType = TUserType;

    static_assert(kIsTrivialRawType<RawType>);
};

template <In TIn, const Name& Name, typename RawType, typename UserType = RawType, typename FormatType = RawType>
struct TrivialParameter {
    static constexpr auto kIn = TIn;
    static constexpr auto& kName = Name;

    using Base = TrivialParameterBase<RawType, UserType, FormatType>;

    static_assert(kIn != In::kQueryExplode);
};

template <
    In TIn,
    char Delimiter,
    typename TRawItemType,
    typename TUserItemType = TRawItemType,
    typename TFormatItemType = TRawItemType>
struct ArrayParameterBase {
    static constexpr char kDelimiter = Delimiter;
    static constexpr auto kIn = TIn;

    using RawType = std::vector<std::string>;
    using RawItemType = TRawItemType;
    using FormatItemType = TFormatItemType;
    using UserType = std::vector<TUserItemType>;
    using UserItemType = TUserItemType;
};

template <
    In TIn,
    const Name& Name,
    char Delimiter,
    typename TRawItemType,
    typename TUserItemType = TRawItemType,
    typename TFormatItemType = TRawItemType>
struct ArrayParameter {
    static constexpr auto& kName = Name;
    static constexpr auto kIn = TIn;

    using Base = ArrayParameterBase<TIn, Delimiter, TRawItemType, TUserItemType, TFormatItemType>;
};

namespace impl {

template <typename Target, typename Source>
Target ConvertIfNeeded(Source&& value) {
    if constexpr (std::is_same_v<std::remove_cvref_t<Source>, Target>) {
        return std::forward<Source>(value);
    } else {
        return Convert(std::forward<Source>(value), convert::To<Target>{});
    }
}

template <typename Base>
typename Base::UserType RawToUser(typename Base::RawType&& raw_value) {
    return impl::ConvertIfNeeded<typename Base::UserType>(
        impl::ConvertIfNeeded<typename Base::FormatType>(std::move(raw_value))
    );
}

template <typename Base>
typename Base::RawType UserToRaw(const typename Base::UserType& value) {
    return impl::ConvertIfNeeded<typename Base::RawType>(impl::ConvertIfNeeded<typename Base::FormatType>(value));
}

}  // namespace impl

}  // namespace chaotic::openapi

USERVER_NAMESPACE_END
