#pragma once

#include <string>
#include <vector>

#include <userver/chaotic/openapi/parameters_write.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi::server {

/// @brief Response-header sink for server handlers.
///
/// Implements `ParameterSinkBase` so that `WriteParameter<Parameter>(value,
/// sink)` can be used to serialize response headers onto the outgoing
/// `HttpResponse`.  Only `SetHeader` is valid for response parameters; all
/// other setter overrides assert-fail to catch mis-routed parameters early.
class ParameterSinkHttpResponse final : public chaotic::openapi::ParameterSinkBase {
public:
    explicit ParameterSinkHttpResponse(USERVER_NAMESPACE::server::http::HttpResponse& response);

    void SetHeader(std::string_view name, std::string&& value) override;

    void SetCookie(std::string_view name, std::string&& value) override;
    void SetPath(chaotic::openapi::Name& name, std::string&& value) override;
    void SetQuery(std::string_view name, std::string&& value) override;
    void SetMultiQuery(std::string_view name, std::vector<std::string>&& value) override;

private:
    USERVER_NAMESPACE::server::http::HttpResponse& response_;
};

}  // namespace chaotic::openapi::server

USERVER_NAMESPACE_END
