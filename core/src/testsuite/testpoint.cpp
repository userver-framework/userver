#include <userver/testsuite/testpoint.hpp>

#include <userver/clients/http/client.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/http/content_type.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite::impl {

TestPoint& TestPoint::GetInstance() {
  static TestPoint tp;
  return tp;
}

void TestPoint::Setup(clients::http::Client& http_client,
                      const std::string& url, std::chrono::milliseconds timeout,
                      bool skip_unregistered_testpoints) {
  UASSERT(!is_initialized_);

  http_client_ = &http_client;
  url_ = url;
  timeout_ = timeout;
  skip_unregistered_testpoints_ = skip_unregistered_testpoints;
  registered_paths_.Assign({});

  is_initialized_ = true;  // seq_cst for http_client_+timeout_
}

void TestPoint::Notify(
    const std::string& name, const formats::json::Value& json,
    const std::function<void(const formats::json::Value&)>& callback) {
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
                      http::content_type::kApplicationJson.ToString()}})
          ->timeout(timeout_)
          ->perform();
  response->raise_for_status();

  if (callback) {
    auto doc = formats::json::FromString(response->body_view());
    callback(doc["data"]);
  }
}

bool TestPoint::IsEnabled() const { return is_initialized_; }

void TestPoint::RegisterPaths(std::unordered_set<std::string> paths) {
  if (skip_unregistered_testpoints_) {
    registered_paths_.Assign(std::move(paths));
  }
}

bool TestPoint::IsRegisteredPath(const std::string& path) const {
  if (skip_unregistered_testpoints_) {
    const auto& paths_set = registered_paths_.Read();
    return paths_set->count(path) > 0;
  } else {
    return true;
  }
}

}  // namespace testsuite::impl

USERVER_NAMESPACE_END
