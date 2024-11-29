#pragma once

#include <userver/chaotic/convert.hpp>
#include <userver/chaotic/openapi/parameters.hpp>
#include <userver/utils/function_ref.hpp>

#include <userver/server/http/http_request.hpp>
#include <userver/utils/from_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi {

/*
 * All parameters are parsed according to the following pipeline:
 *
 *   [easy] -> str -> raw -> user
 *
 * Where:
 *  - [easy] is curl easy handler
 *  - str is `string` or `vector of strings`
 *  - raw is one of JSON Schema types (e.g. boolean, integer or string)
 *  - user is a type shown to the user (x-usrv-cpp-type value or same as raw)
 *
 */

template <In In_>
auto GetParameter(std::string_view name, const server::http::HttpRequest& source) {
    if constexpr (In_ == In::kPath) {
        return source.GetPathArg(name);
    } else if constexpr (In_ == In::kCookie) {
        return source.GetCookie(std::string{name});
    } else if constexpr (In_ == In::kHeader) {
        return source.GetHeader(name);
    } else if constexpr (In_ == In::kQuery) {
        return source.GetArg(name);
    } else {
        static_assert(In_ == In::kQueryExplode, "Unknown 'In'");
        return source.GetArgVector(name);
    }
}

namespace parse {
template <typename T>
struct To {};
}  // namespace parse

template <typename T>
std::enable_if_t<std::is_integral_v<T>, T> FromStr(std::string&& s, parse::To<T>) {
    return utils::FromString<T>(s);
}

std::string FromStr(std::string&& str_value, parse::To<std::string>);

bool FromStr(std::string&& str_value, parse::To<bool>);

double FromStr(std::string&& str_value, parse::To<double>);

template <typename Parameter>
struct ParseParameter {
    static std::string Parse(typename Parameter::RawType&& t) {
        static_assert(sizeof(t) && false, "Cannot find `ParseParameter`");
        return {};
    }
};

template <typename RawType, typename UserType>
struct ParseParameter<TrivialParameterBase<RawType, UserType>> {
    static UserType Parse(std::string&& str_value) {
        auto raw_value = openapi::FromStr(std::move(str_value), parse::To<RawType>());
        return Convert(std::move(raw_value), convert::To<UserType>());
    }
};

namespace impl {

void SplitByDelimiter(std::string_view str, char delimiter, utils::function_ref<void(std::string)>);

}

template <In In_, char Delimiter, typename RawType, typename UserType>
struct ParseParameter<ArrayParameterBase<In_, Delimiter, RawType, UserType>> {
    static auto Parse(std::string&& str_value) {
        openapi::ParseParameter<TrivialParameterBase<RawType, UserType>> parser;

        std::vector<UserType> result;
        impl::SplitByDelimiter(str_value, Delimiter, [&result, &parser](std::string str) {
            result.emplace_back(parser.Parse(std::move(str)));
        });
        return result;
    }
};

template <char Delimiter, typename RawType, typename UserType>
struct ParseParameter<ArrayParameterBase<In::kQueryExplode, Delimiter, RawType, UserType>> {
    static auto Parse(std::vector<std::string>&& str_value) {
        openapi::ParseParameter<TrivialParameterBase<RawType, UserType>> parser;

        std::vector<UserType> result;
        result.reserve(str_value.size());
        for (auto&& str_item : str_value) {
            result.emplace_back(parser.Parse(std::move(str_item)));
        }
        return result;
    }
};

template <typename Parameter>
typename Parameter::Base::UserType ReadParameter(const server::http::HttpRequest& source) {
    auto str_or_array_value = openapi::GetParameter<Parameter::kIn>(Parameter::kName, source);
    return openapi::ParseParameter<typename Parameter::Base>::Parse(std::move(str_or_array_value));
}

}  // namespace chaotic::openapi

USERVER_NAMESPACE_END
