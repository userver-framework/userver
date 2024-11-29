#include <userver/chaotic/openapi/parameters_write.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi {

ParameterSinkHttpClient::ParameterSinkHttpClient(clients::http::Request& request, std::string&& url_pattern)
    : url_pattern_(std::move(url_pattern)), request_(request) {}

void ParameterSinkHttpClient::SetCookie(std::string_view name, std::string&& value) {
    cookies_.emplace(name, std::move(value));
}

void ParameterSinkHttpClient::SetHeader(std::string_view name, std::string&& value) {
    headers_.emplace(name, std::move(value));
}

void ParameterSinkHttpClient::SetPath(Name& name, std::string&& value) {
    // Note: passing tmp value to fmt::arg() is OK and not a UAF
    // since fmt::dynamic_format_arg_store copies the data into itself.
    path_vars_.push_back(fmt::arg(name, std::move(value)));
}

void ParameterSinkHttpClient::SetQuery(std::string_view name, std::string&& value) {
    query_args_.insert({std::string{name}, std::move(value)});
}

void ParameterSinkHttpClient::SetMultiQuery(std::string_view name, std::vector<std::string>&& value) {
    for (auto&& item : value) {
        query_args_.insert({std::string{name}, std::move(item)});
    }
}

void ParameterSinkHttpClient::Flush() {
    request_.url(http::MakeUrl(fmt::vformat(url_pattern_, path_vars_), {}, query_args_));
    request_.headers(std::move(headers_));
    request_.cookies(std::move(cookies_));
}

std::string ToStrParameter(bool value) noexcept { return value ? "true" : "false"; }

std::string ToStrParameter(double value) noexcept { return fmt::to_string(value); }

std::string ToStrParameter(std::string&& s) noexcept { return std::move(s); }

std::vector<std::string> ToStrParameter(std::vector<std::string>&& s) noexcept { return std::move(s); }

void ValidatePathVariableValue(std::string_view name, std::string_view value) {
    if (value.find('/') != std::string::npos || value.find('?') != std::string::npos) {
        throw std::runtime_error(fmt::format("Forbidden symbol in path variable value: {}='{}'", name, value));
    }
}

}  // namespace chaotic::openapi

USERVER_NAMESPACE_END
