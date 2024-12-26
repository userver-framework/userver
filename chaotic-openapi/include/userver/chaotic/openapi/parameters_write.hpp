#pragma once

#include <fmt/args.h>
#include <fmt/ranges.h>

#include <userver/chaotic/convert.hpp>
#include <userver/chaotic/openapi/parameters.hpp>
#include <userver/clients/http/request.hpp>
#include <userver/http/url.hpp>
#include <userver/utils/impl/projecting_view.hpp>
#include <userver/utils/meta.hpp>
#include <userver/utils/text_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi {

/*
 * All parameters are serialized according to the following pipeline:
 *
 *   user -> raw -> str -> [easy]
 *
 * Where:
 *  - user is a type shown to the user (x-usrv-cpp-type value or same as raw)
 *  - raw is one of JSON Schema types (e.g. boolean, integer or string)
 *  - str is `string` or `vector of strings`
 *  - [easy] is curl easy handler
 *
 */

/// @brief curl::easy abstraction
class ParameterSinkBase {
public:
    virtual void SetCookie(std::string_view, std::string&&) = 0;
    virtual void SetHeader(std::string_view, std::string&&) = 0;
    virtual void SetPath(Name&, std::string&&) = 0;
    virtual void SetQuery(std::string_view, std::string&&) = 0;
    virtual void SetMultiQuery(std::string_view, std::vector<std::string>&&) = 0;
};

class ParameterSinkHttpClient final : public ParameterSinkBase {
public:
    ParameterSinkHttpClient(clients::http::Request& request, std::string&& url_pattern);

    void SetCookie(std::string_view name, std::string&& value) override;

    void SetHeader(std::string_view name, std::string&& value) override;

    void SetPath(Name& name, std::string&& value) override;

    void SetQuery(std::string_view name, std::string&& value) override;

    void SetMultiQuery(std::string_view name, std::vector<std::string>&& value) override;

    void Flush();

private:
    std::string url_pattern_;
    clients::http::Request& request_;
    clients::http::Headers headers_;
    http::MultiArgs query_args_;
    std::unordered_map<std::string, std::string> cookies_;
    fmt::dynamic_format_arg_store<fmt::format_context> path_vars_;
};

void ValidatePathVariableValue(std::string_view name, std::string_view value);

template <In In, typename StrType>
void SetParameter(Name& name, StrType&& str_value, ParameterSinkBase& dest) {
    static_assert(std::is_same_v<StrType, std::string> || std::is_same_v<StrType, std::vector<std::string>>);

    if constexpr (In == In::kPath) {
        USERVER_NAMESPACE::chaotic::openapi::ValidatePathVariableValue(name, str_value);
        dest.SetPath(name, std::forward<StrType>(str_value));
    } else if constexpr (In == In::kCookie) {
        dest.SetCookie(name, std::forward<StrType>(str_value));
    } else if constexpr (In == In::kHeader) {
        dest.SetHeader(name, std::forward<StrType>(str_value));
    } else if constexpr (In == In::kQuery) {
        dest.SetQuery(name, std::forward<StrType>(str_value));
    } else {
        static_assert(In == In::kQueryExplode, "Unknown 'In'");
        dest.SetMultiQuery(name, std::forward<StrType>(str_value));
    }
}

template <typename T>
std::enable_if_t<std::is_integral_v<T>, std::string> ToStrParameter(T s) noexcept {
    return std::to_string(s);
}

template <typename T>
std::enable_if_t<std::is_enum_v<T>, std::string> ToStrParameter(T s) noexcept {
    return ToString(s);
}

std::string ToStrParameter(bool value) noexcept;

std::string ToStrParameter(double value) noexcept;

std::string ToStrParameter(std::string&& s) noexcept;

std::vector<std::string> ToStrParameter(std::vector<std::string>&& s) noexcept;

template <typename T>
std::enable_if_t<std::is_same_v<meta::RangeValueType<T>, std::string>, std::vector<std::string>> ToStrParameter(
    T&& collection
) noexcept {
    std::vector<std::string> result;
    result.reserve(collection.size());
    for (auto&& item : collection) {
        result.emplace_back(ToStrParameter(item));
    }
    return result;
}

template <typename Parameter>
struct SerializeParameter {
    static std::string Serialize(const typename Parameter::UserType&) {
        static_assert(sizeof(Parameter) && false, "No SerializeParameter specialization found for `Parameter`");
        return {};
    }
};

template <typename T>
struct SerializeParameter<TrivialParameterBase<T, T>> {
    static auto Serialize(const T& value) { return ToStrParameter(T{value}); }
};

template <typename RawType, typename UserType>
struct SerializeParameter<TrivialParameterBase<RawType, UserType>> {
    static auto Serialize(const UserType& value) { return ToStrParameter(Convert(value, convert::To<RawType>{})); }
};

template <char Delimiter, typename T, typename U>
struct SerializeParameter<ArrayParameterBase<In::kQueryExplode, Delimiter, T, U>> {
    static std::vector<std::string> Serialize(const std::vector<U>& collection) {
        // Note: ignore Delimiter
        std::vector<std::string> result;
        result.reserve(collection.size());
        for (const auto& item : collection) {
            result.emplace_back(SerializeParameter<TrivialParameterBase<T, U>>().Serialize(item));
        }
        return result;
    }
};

template <In In_, char Delimiter, typename RawType, typename UserType>
struct SerializeParameter<ArrayParameterBase<In_, Delimiter, RawType, UserType>> {
    static_assert(In_ != In::kQueryExplode);

    std::string operator()(const UserType& item) const { return ToStrParameter(Convert(item, convert::To<RawType>{})); }

    static std::string Serialize(const std::vector<UserType>& value) {
        using ProjectingView = utils::impl::ProjectingView<const std::vector<UserType>, SerializeParameter>;
        return fmt::to_string(fmt::join(ProjectingView{value, SerializeParameter{}}, std::string(1, Delimiter)));
    }
};

template <typename Parameter>
void WriteParameter(const typename Parameter::Base::UserType& value, ParameterSinkBase& dest) {
    auto str_value = openapi::SerializeParameter<typename Parameter::Base>::Serialize(value);
    openapi::SetParameter<Parameter::kIn>(Parameter::kName, std::move(str_value), dest);
}

}  // namespace chaotic::openapi

USERVER_NAMESPACE_END
