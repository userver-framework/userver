#include <userver/testsuite/http_testpoint_client.hpp>

#include <utility>

#include <userver/clients/http/client.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/http/content_type.hpp>
#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite::impl {

HttpTestpointClient::HttpTestpointClient(clients::http::Client& http_client,
                                         const std::string& url,
                                         std::chrono::milliseconds timeout)
    : http_client_(http_client), url_(url), timeout_(timeout) {}

HttpTestpointClient::~HttpTestpointClient() { Unregister(); }

void HttpTestpointClient::Execute(const std::string& name,
                                  const formats::json::Value& json,
                                  const Callback& callback) const {
  tracing::Span span("testpoint");
  const auto& testpoint_id = span.GetSpanId();
  const auto& data = formats::json::ToString(json);

  span.AddTag("testpoint_id", testpoint_id);
  span.AddTag("testpoint", name);

  formats::json::ValueBuilder request;
  request["id"] = testpoint_id;
  request["name"] = name;
  request["data"] = json;
  auto request_str = formats::json::ToString(request.ExtractValue());

  LOG_INFO() << "Running testpoint " << name << ": " << data;

  auto response =
      http_client_.CreateRequest()
          .post(url_, std::move(request_str))
          .headers({{http::headers::kContentType,
                     http::content_type::kApplicationJson.ToString()}})
          .timeout(timeout_)
          .perform();
  response->raise_for_status();

  if (callback) {
    auto doc = formats::json::FromString(response->body_view());
    if (doc["handled"].As<bool>(true)) {
      callback(doc["data"]);
    }
  }
}

}  // namespace testsuite::impl

USERVER_NAMESPACE_END
