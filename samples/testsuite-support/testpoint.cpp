#include "testpoint.hpp"

#include <userver/testsuite/testpoint.hpp>

namespace tests::handlers {

formats::json::Value Testpoint::HandleRequestJsonThrow(
    [[maybe_unused]] const server::http::HttpRequest& request,
    [[maybe_unused]] const formats::json::Value& request_body,
    [[maybe_unused]] server::request::RequestContext& context) const {
  formats::json::ValueBuilder result;

  TESTPOINT("simple-testpoint", [] {
    formats::json::ValueBuilder builder;
    builder["payload"] = "Hello, world!";
    return builder.ExtractValue();
  }());

  TESTPOINT_CALLBACK("injection-point", formats::json::Value(),
                     [&result](const formats::json::Value& doc) {
                       if (doc.IsObject()) {
                         result["value"] = doc["value"].As<std::string>("");
                       }
                     });

  return result.ExtractValue();
}

}  // namespace tests::handlers
