#include "testpoint.hpp"

/// [Testpoint - include]
#include <userver/testsuite/testpoint.hpp>
/// [Testpoint - include]

namespace tests::handlers {

formats::json::Value Testpoint::HandleRequestJsonThrow(
    [[maybe_unused]] const server::http::HttpRequest& request,
    [[maybe_unused]] const formats::json::Value& request_body,
    [[maybe_unused]] server::request::RequestContext& context) const {
  formats::json::ValueBuilder result;

  /// [Testpoint - TESTPOINT()]
  TESTPOINT("simple-testpoint", [] {
    formats::json::ValueBuilder builder;
    builder["payload"] = "Hello, world!";
    return builder.ExtractValue();
  }());
  /// [Testpoint - TESTPOINT()]

  TESTPOINT_CALLBACK("injection-point", formats::json::Value(),
                     [&result](const formats::json::Value& doc) {
                       if (doc.IsObject()) {
                         result["value"] = doc["value"].As<std::string>("");
                       }
                     });

  return result.ExtractValue();
}

}  // namespace tests::handlers
