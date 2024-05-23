#pragma once

#include <userver/telegram/bot/utils/token.hpp>

#include <userver/clients/http/request.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// todo
struct RequestOptions {
  /// todo
  std::chrono::milliseconds timeout = std::chrono::milliseconds{500};

  /// todo
  size_t retries = 2;
};

template <typename method>
class [[nodiscard]] Request {
 public:
  using Method = method;

  Request(clients::http::Request&& request,
          std::string_view base_url,
          std::string_view bot_token,
          const RequestOptions& request_options,
          const typename Method::Parameters& parameters)
      : http_request_(std::move(request)) {
    SetUrl(base_url, bot_token);
    SetRequestOptions(request_options);
    Method::FillRequestData(http_request_, parameters);
  }

  typename Method::Reply Perform() {
    auto response = http_request_.perform();
    return Method::ParseResponseData(*response);
  }

 private:
  void SetRequestOptions(const RequestOptions& request_options) {
    http_request_.timeout(request_options.timeout)
                 .retry(request_options.retries);
  }

  void SetUrl(std::string_view base_url, std::string_view raw_token) {
    Token token{raw_token};

    http_request_.url(GetFullUrl(base_url, token.GetToken()))
                 .SetLoggedUrl(GetFullUrl(base_url, token.GetHiddenToken()));
  }

  std::string GetFullUrl(std::string_view base_url,
                         std::string_view token,
                         bool is_test_env = false) {
    if (!is_test_env) {
      return fmt::format("{}/bot{}/{}", base_url, token, Method::kName);
    } else {
      return fmt::format("{}/bot{}/test/{}", base_url, token, Method::kName);
    }
  }

  clients::http::Request http_request_;
};


}  // namespace telegram::bot

USERVER_NAMESPACE_END
