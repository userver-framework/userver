#pragma once

#include <concepts>
#include <optional>

#include <fmt/format.h>

#include <userver/chaotic/convert.hpp>
#include <userver/chaotic/openapi/parameters.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/utils/from_string.hpp>
#include <userver/utils/function_ref.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi {

/// @brief ADL tag for `ParseRequest(HttpRequest, To<Request>)`.
///
/// Adding `To<Request>` as a second argument makes ADL search the namespace
/// of `Request` (the handler's namespace), where `ParseRequest` is defined.
template <typename T>
struct To {};

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

template <In kIn>
auto GetParameter(std::string_view name, const server::http::HttpRequest& source) {
    if constexpr (kIn == In::kPath) {
        return source.GetPathArg(name);
    } else if constexpr (kIn == In::kCookie) {
        return source.GetCookie(std::string{name});
    } else if constexpr (kIn == In::kHeader) {
        return source.GetHeader(name);
    } else if constexpr (kIn == In::kQuery) {
        return source.GetArg(name);
    } else {
        static_assert(kIn == In::kQueryExplode, "Unknown 'In'");
        return source.GetArgVector(name);
    }
}

/// Returns true if the parameter is present in the request.
///
/// NOTE: for headers and cookies, an explicitly-provided empty value ("") is
/// indistinguishable from an absent one and is treated as absent.
template <In kIn>
bool IsParameterPresent(std::string_view name, const server::http::HttpRequest& source) {
    if constexpr (kIn == In::kPath) {
        return true;  // routing guarantees path parameters are always present
    } else if constexpr (kIn == In::kQuery) {
        return source.HasArg(name);
    } else if constexpr (kIn == In::kQueryExplode) {
        return !source.GetArgVector(name).empty();
    } else if constexpr (kIn == In::kHeader) {
        return source.HasHeader(name);
    } else {
        static_assert(kIn == In::kCookie, "Unknown 'In'");
        return source.HasCookie(std::string{name});
    }
}

namespace parse {
template <typename T>
struct To {};
}  // namespace parse

template <std::integral T>
T ParseParameter(std::string&& s, parse::To<T>) {
    return utils::FromString<T>(s);
}

std::string ParseParameter(std::string&& str_value, parse::To<std::string>);

bool ParseParameter(std::string&& str_value, parse::To<bool>);

double ParseParameter(std::string&& str_value, parse::To<double>);

template <typename Parameter>
struct ParameterParser {
    static std::string Parse(typename Parameter::RawType&& t) {
        static_assert(!sizeof(t), "Cannot find `ParameterParser`");
        return {};
    }
};

template <typename RawType, typename UserType>
struct ParameterParser<TrivialParameterBase<RawType, UserType>> {
    static UserType Parse(std::string&& str_value) {
        auto raw_value = openapi::ParseParameter(std::move(str_value), parse::To<RawType>());
        return Convert(std::move(raw_value), convert::To<UserType>());
    }
};

namespace impl {

void SplitByDelimiter(std::string_view str, char delimiter, utils::function_ref<void(std::string)>);

}

template <In kIn, char Delimiter, typename RawType, typename UserType>
struct ParameterParser<ArrayParameterBase<kIn, Delimiter, RawType, UserType>> {
    static auto Parse(std::string&& str_value) {
        openapi::ParameterParser<TrivialParameterBase<RawType, UserType>> parser;

        std::vector<UserType> result;
        impl::SplitByDelimiter(str_value, Delimiter, [&result, &parser](std::string str) {
            result.emplace_back(parser.Parse(std::move(str)));
        });
        return result;
    }
};

template <char Delimiter, typename RawType, typename UserType>
struct ParameterParser<ArrayParameterBase<In::kQueryExplode, Delimiter, RawType, UserType>> {
    static auto Parse(std::vector<std::string>&& str_value) {
        openapi::ParameterParser<TrivialParameterBase<RawType, UserType>> parser;

        std::vector<UserType> result;
        result.reserve(str_value.size());
        for (auto&& str_item : str_value) {
            result.emplace_back(parser.Parse(std::move(str_item)));
        }
        return result;
    }
};

/// Reads a required parameter. Throws server::handlers::ClientError if the
/// parameter is absent from the request.
template <typename Parameter>
typename Parameter::Base::UserType ReadParameter(const server::http::HttpRequest& source) {
    if (!openapi::IsParameterPresent<Parameter::kIn>(Parameter::kName, source)) {
        throw server::handlers::ClientError(server::handlers::ExternalBody{
            fmt::format("Required parameter '{}' is missing", Parameter::kName)
        });
    }
    auto str_or_array_value = openapi::GetParameter<Parameter::kIn>(Parameter::kName, source);
    return openapi::ParameterParser<typename Parameter::Base>::Parse(std::move(str_or_array_value));
}

/// Reads an optional parameter. Returns std::nullopt if the parameter is
/// absent; returns the parsed value (even if the raw string is empty) if
/// present.
template <typename Parameter>
std::optional<typename Parameter::Base::UserType> ReadParameterOptional(const server::http::HttpRequest& source) {
    if (!openapi::IsParameterPresent<Parameter::kIn>(Parameter::kName, source)) {
        return std::nullopt;
    }
    auto str_or_array_value = openapi::GetParameter<Parameter::kIn>(Parameter::kName, source);
    return openapi::ParameterParser<typename Parameter::Base>::Parse(std::move(str_or_array_value));
}

}  // namespace chaotic::openapi

USERVER_NAMESPACE_END
