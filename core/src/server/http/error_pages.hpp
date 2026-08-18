#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <userver/server/http/http_status.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

class HttpResponse;

/// A substitute response for the errors that the server reports by itself,
/// an analog of the nginx `error_page` directive.
struct ErrorPage final {
    /// Status to respond with instead of the original one.
    std::optional<HttpStatus> status;
    /// Body to respond with instead of the server-generated one.
    std::optional<std::string> body;
    /// Headers to set on the response, in the configuration order.
    std::vector<std::pair<std::string, std::string>> headers;
};

/// Error pages of a single listener, indexed by the original error status.
class ErrorPages final {
public:
    ErrorPages() = default;
    explicit ErrorPages(std::unordered_map<HttpStatus, ErrorPage> pages);

    /// @returns the page configured for `status`, or `nullptr` if there is none.
    const ErrorPage* Find(HttpStatus status) const noexcept;

private:
    std::unordered_map<HttpStatus, ErrorPage> pages_;
};

/// Overrides the status, the body and the headers of `response` as `page` says.
void ApplyErrorPage(const ErrorPage& page, HttpResponse& response);

ErrorPages Parse(const yaml_config::YamlConfig& value, formats::parse::To<ErrorPages>);

}  // namespace server::http

USERVER_NAMESPACE_END
