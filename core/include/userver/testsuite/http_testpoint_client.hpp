#pragma once

#include <chrono>
#include <functional>
#include <string>

#include <userver/formats/json_fwd.hpp>
#include <userver/testsuite/testpoint_control.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {
class Client;
}  // namespace clients::http

namespace testsuite::impl {

class HttpTestpointClient final : public TestpointClientBase {
 public:
  HttpTestpointClient(clients::http::Client& http_client,
                      const std::string& url,
                      std::chrono::milliseconds timeout);

  ~HttpTestpointClient() override;

  void Execute(const std::string& name, const formats::json::Value& json,
               const Callback& callback) const override;

 private:
  clients::http::Client& http_client_;
  const std::string url_;
  const std::chrono::milliseconds timeout_;
};

}  // namespace testsuite::impl

USERVER_NAMESPACE_END
