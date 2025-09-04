#include <userver/chaotic/openapi/parameters_write.hpp>

#include <boost/range/adaptor/transformed.hpp>

#include <fmt/format.h>

#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi {

namespace {

constexpr std::string_view kMask = "***";

auto MaskQueryMultiArgs(const http::MultiArgs& args, ParameterSinkHttpClient::HiddenQueryArgNamesFunc func) {
    using Pair = std::pair<std::string_view, std::string_view>;

    auto masked = args | boost::adaptors::transformed([&func](const auto& pair) {
                      const auto& [name, value] = pair;
                      if (func(name))
                          return Pair(name, kMask);
                      else
                          return Pair(name, value);
                  });
    return utils::AsContainer<std::vector<Pair>>(std::move(masked));
}

}  // namespace

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
    if (hidden_query_arg_names_func_) {
        auto logged_query_args = MaskQueryMultiArgs(query_args_, hidden_query_arg_names_func_);
        auto url = http::MakeUrl(fmt::vformat(url_pattern_, path_vars_), logged_query_args);
        request_.SetLoggedUrl(url);
    }
    request_.headers(std::move(headers_));
    request_.cookies(std::move(cookies_));
}

const clients::http::Headers& ParameterSinkHttpClient::GetHeaders() const { return headers_; }

void ParameterSinkHttpClient::SetHiddenQueryArgNamesFunc(HiddenQueryArgNamesFunc func) {
    hidden_query_arg_names_func_ = func;
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
