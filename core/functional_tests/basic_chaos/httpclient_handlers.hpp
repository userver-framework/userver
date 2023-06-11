#pragma once

#include <userver/clients/http/component.hpp>
#include <userver/clients/http/streamed_response.hpp>
#include <userver/components/component_context.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_response_body_stream.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/utest/using_namespace_userver.hpp>

namespace chaos {

const int kTimeoutSecs{15};

class HttpClientHandler final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-chaos-httpclient";

  HttpClientHandler(const components::ComponentConfig& config,
                    const components::ComponentContext& context)
      : HttpHandlerBase(config, context),
        client_(
            context.FindComponent<components::HttpClient>().GetHttpClient()) {}

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override {
    const auto& type = request.GetArg("type");
    const auto& port = request.GetArg("port");
    const auto& timeout = request.GetArg("timeout");
    const std::chrono::seconds timeout_secs{
        timeout.empty() ? kTimeoutSecs : std::stoi(timeout)};
    if (type == "common") {
      auto url = fmt::format("http://localhost:{}/test", port);
      auto response = client_.CreateNotSignedRequest()
                          .get(url)
                          .timeout(timeout_secs)
                          .retry(1)
                          .perform();
      response->raise_for_status();
      return response->body();
    }

    UASSERT(false);
    return {};
  }

 private:
  clients::http::Client& client_;
};

class StreamHandler : public server::handlers::HttpHandlerBase {
 public:
  // `kName` must match component name in config.yaml
  static constexpr std::string_view kName = "handler-chaos-stream";

  StreamHandler(const components::ComponentConfig& config,
                const components::ComponentContext& context)
      : server::handlers::HttpHandlerBase(config, context),
        http_client_(
            context.FindComponent<components::HttpClient>().GetHttpClient()),
        config_source_(
            context.FindComponent<components::DynamicConfig>().GetSource()) {}

  /// [HandleStreamRequest]
  void HandleStreamRequest(
      const server::http::HttpRequest& request,
      server::request::RequestContext&,
      server::http::ResponseBodyStream& response_body_stream) const override {
    ::clients::http::Headers headers;
    for (const auto& header_name : request.GetHeaderNames()) {
      const auto& header_value = request.GetHeader(header_name);
      headers[header_name] = header_value;
    }

    const auto& port = request.GetArg("port");
    auto url = fmt::format("http://localhost:{}/test", port);
    const auto& timeout = request.GetArg("timeout");
    const std::chrono::seconds timeout_secs{
        timeout.empty() ? kTimeoutSecs : std::stoi(timeout)};
    const auto retries = 1;

    auto external_request = http_client_.CreateNotSignedRequest()
                                .get(url)
                                .headers(std::move(headers))
                                .timeout(timeout_secs)
                                .retry(retries);

    auto queue = concurrent::StringStreamQueue::Create();
    auto client_response =
        external_request.async_perform_stream_body(std::move(queue));

    TESTPOINT("stream_after_start", {});

    for (const auto& header_item : client_response.GetHeaders()) {
      TESTPOINT("stream_after_get_headers", {});
      const auto& header_name = header_item.first;
      const auto& header_value = header_item.second;
      response_body_stream.SetHeader(header_name, header_value);
    }

    auto status_code = client_response.StatusCode();
    response_body_stream.SetStatusCode(status_code);

    response_body_stream.SetHeader(std::string{"abc"}, std::string{"def"});
    response_body_stream.SetEndOfHeaders();

    TESTPOINT("stream_after_set_end_of_headers", {});

    std::string body_part;
    auto deadline = engine::Deadline::FromDuration(std::chrono::seconds(10));
    while (client_response.ReadChunk(body_part, deadline)) {
      TESTPOINT("stream_after_read_chunk", {});
      response_body_stream.PushBodyChunk(std::move(body_part),
                                         engine::Deadline());
      TESTPOINT("stream_after_push_body_chunk", {});
    }
  }
  /// [HandleStreamRequest]

 private:
  clients::http::Client& http_client_;
  dynamic_config::Source config_source_;
};

}  // namespace chaos
