#include <utils/testpoint.hpp>

#include <clients/http/client.hpp>
#include <formats/json/serialize.hpp>
#include <formats/json/value_builder.hpp>
#include <http/common_headers.hpp>
#include <server/http/content_type.hpp>
#include <tracing/span.hpp>

namespace utils {

TestPoint& TestPoint::GetInstance() {
  static TestPoint tp;
  return tp;
}

void TestPoint::Setup(clients::http::Client& http_client,
                      const std::string& url,
                      std::chrono::milliseconds timeout) {
  UASSERT(!is_initialized_);

  http_client_ = &http_client;
  url_ = url;
  timeout_ = timeout;

  is_initialized_ = true;  // seq_cst for http_client_+timeout_
}

void TestPoint::Notify(const std::string& name,
                       const formats::json::Value& json) {
  if (!is_initialized_) return;

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
      http_client_->CreateRequest()
          ->post(url_, request_str)
          ->headers({{http::headers::kContentType,
                      server::http::content_type::kApplicationJson}})
          ->timeout(timeout_)
          ->perform();
  response->raise_for_status();
}

bool TestPoint::IsEnabled() const { return is_initialized_; }

}  // namespace utils
