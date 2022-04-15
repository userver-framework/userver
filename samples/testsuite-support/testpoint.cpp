#include "testpoint.hpp"

/// [Testpoint - include]
#include <userver/testsuite/testpoint.hpp>
/// [Testpoint - include]

namespace tests::handlers {

formats::json::Value Testpoint::HandleRequestJsonThrow(
    [[maybe_unused]] const server::http::HttpRequest& request,
    [[maybe_unused]] const formats::json::Value& request_body,
    [[maybe_unused]] server::request::RequestContext& context) const {
  /// [Testpoint - TESTPOINT()]
  TESTPOINT("simple-testpoint", [] {
    formats::json::ValueBuilder builder;
    builder["payload"] = "Hello, world!";
    return builder.ExtractValue();
  }());
  /// [Testpoint - TESTPOINT()]

  /// [Sample TESTPOINT_CALLBACK usage cpp]
  formats::json::ValueBuilder result;

  TESTPOINT_CALLBACK("injection-point", formats::json::Value(),
                     [&result](const formats::json::Value& doc) {
                       result["value"] = doc["value"].As<std::string>("");
                     });
  /// [Sample TESTPOINT_CALLBACK usage cpp]

  return result.ExtractValue();
}

}  // namespace tests::handlers
