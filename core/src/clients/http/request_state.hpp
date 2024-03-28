#pragma once

#include <array>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>
#include <system_error>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/clients/http/config.hpp>
#include <userver/clients/http/error.hpp>
#include <userver/clients/http/form.hpp>
#include <userver/clients/http/plugin.hpp>
#include <userver/clients/http/request_tracing_editor.hpp>
#include <userver/clients/http/response_future.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/crypto/certificate.hpp>
#include <userver/crypto/private_key.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/future.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/http/url.hpp>
#include <userver/tracing/in_place_span.hpp>
#include <userver/tracing/manager.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/not_null.hpp>

#include <clients/http/destination_statistics.hpp>
#include <clients/http/easy_wrapper.hpp>
#include <clients/http/testsuite.hpp>
#include <crypto/helpers.hpp>
#include <engine/ev/watcher/timer_watcher.hpp>
#include <server/http/headers_propagator.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

class StreamedResponse;
class ConnectTo;

class RequestState : public std::enable_shared_from_this<RequestState> {
 public:
  RequestState(impl::EasyWrapper&&, RequestStats&& req_stats,
               const std::shared_ptr<DestinationStatistics>& dest_stats,
               clients::dns::Resolver* resolver,
               impl::PluginPipeline& plugin_pipeline,
               const tracing::TracingManagerBase& tracing_manager);
  ~RequestState();

  using Queue = concurrent::StringStreamQueue;

  /// Perform async http request
  engine::Future<std::shared_ptr<Response>> async_perform(
      utils::impl::SourceLocation location =
          utils::impl::SourceLocation::Current());

  /// Perform streaming http request, returns headers future
  engine::Future<void> async_perform_stream(
      const std::shared_ptr<Queue>& queue,
      utils::impl::SourceLocation location =
          utils::impl::SourceLocation::Current());

  /// set redirect flags
  void follow_redirects(bool follow);
  /// set verify flags
  void verify(bool verify);
  /// set file holding one or more certificates to verify the peer with
  void ca_info(const std::string& file_path);
  /// set certificate to verify the peer with
  void ca(crypto::Certificate cert);
  /// set CRL-file
  void crl_file(const std::string& file_path);
  /// set private key and certificate from memory
  void client_key_cert(crypto::PrivateKey pkey, crypto::Certificate cert);
  /// Set HTTP version
  void http_version(curl::easy::http_version_t version);
  /// set timeout value
  void set_timeout(long timeout_ms);
  /// set number of retries
  void retry(short retries, bool on_fails);
  /// set unix socket as transport instead of TCP
  void unix_socket_path(const std::string& path);
  /// set connect_to option
  void connect_to(const ConnectTo& connect_to);
  /// sets proxy to use
  void proxy(const std::string& value);
  /// sets proxy auth type to use
  void proxy_auth_type(curl::easy::proxyauth_t value);
  /// sets proxy auth type and credentials to use
  void http_auth_type(curl::easy::httpauth_t value, bool auth_only,
                      std::string_view user, std::string_view password);

  /// get timeout value in milliseconds
  long timeout() const { return original_timeout_.count(); }
  /// get retries count
  short retries() const { return retry_.retries; }

  engine::Deadline GetDeadline() const noexcept;
  /// true iff *we detected* that the deadline has expired
  bool IsDeadlineExpired() const noexcept;

  [[noreturn]] void ThrowDeadlineExpiredException();

  /// cancel request
  void Cancel();

  void SetDestinationMetricNameAuto(std::string destination);

  void SetDestinationMetricName(const std::string& destination);

  void SetTestsuiteConfig(const std::shared_ptr<const TestsuiteConfig>& config);

  void SetAllowedUrlsExtra(const std::vector<std::string>& urls);

  void DisableReplyDecoding();

  void SetCancellationPolicy(CancellationPolicy cp);

  CancellationPolicy GetCancellationPolicy() const;

  void SetDeadlinePropagationConfig(
      const DeadlinePropagationConfig& deadline_propagation_config);

  curl::easy& easy() { return easy_.Easy(); }
  const curl::easy& easy() const { return easy_.Easy(); }
  std::shared_ptr<Response> response() const { return response_; }
  std::shared_ptr<Response> response_move() { return std::move(response_); }

  void SetLoggedUrl(std::string url);
  void SetEasyTimeout(std::chrono::milliseconds timeout);

  void SetTracingManager(const tracing::TracingManagerBase&);
  void SetHeadersPropagator(const server::http::HeadersPropagator*);

  RequestTracingEditor GetEditableTracingInstance();

 private:
  /// final callback that calls user callback and set value in promise
  static void on_completed(std::shared_ptr<RequestState>, std::error_code err);
  /// retry callback
  static void on_retry(std::shared_ptr<RequestState>, std::error_code err);
  /// header function curl callback
  static size_t on_header(void* ptr, size_t size, size_t nmemb, void* userdata);

  /// certificate function curl callback
  static curl::native::CURLcode on_certificate_request(void* curl, void* sslctx,
                                                       void* userdata) noexcept;

  /// parse one header
  void parse_header(char* ptr, size_t size);
  void ParseSingleCookie(const char* ptr, size_t size);
  /// simply run perform_request if there is now errors from timer
  void on_retry_timer(std::error_code err);
  /// run curl async_request, called once per attempt
  void perform_request(curl::easy::handler_type handler);

  void UpdateTimeoutFromDeadline(std::chrono::milliseconds backoff);
  [[nodiscard]] bool UpdateTimeoutFromDeadlineAndCheck(
      std::chrono::milliseconds backoff = {});
  void UpdateTimeoutHeader();
  void HandleDeadlineAlreadyPassed();
  void CheckResponseDeadline(std::error_code& err, Status status_code);
  bool IsDeadlineExpiredResponse(Status status_code);
  bool ShouldRetryResponse();

  const std::string& GetLoggedOriginalUrl() const noexcept;

  static size_t StreamWriteFunction(char* ptr, size_t size, size_t nmemb,
                                    void* userdata);

  void AccountResponse(std::error_code err);
  std::exception_ptr PrepareException(std::error_code err);

  void ResetDataForNewRequest();
  void ApplyTestsuiteConfig();
  void StartNewSpan(utils::impl::SourceLocation location);
  void StartStats();

  template <typename Func>
  void WithRequestStats(const Func& func);

  void ResolveTargetAddress(clients::dns::Resolver& resolver);

  /// curl handler wrapper
  impl::EasyWrapper easy_;
  RequestStats stats_;
  std::shared_ptr<RequestStats> dest_req_stats_;
  CancellationPolicy cancellation_policy_{CancellationPolicy::kCancel};

  std::shared_ptr<DestinationStatistics> dest_stats_;
  std::string destination_metric_name_;

  std::shared_ptr<const TestsuiteConfig> testsuite_config_;
  std::vector<std::string> allowed_urls_extra_;

  crypto::PrivateKey pkey_;
  crypto::Certificate cert_;
  crypto::Certificate ca_;

  /// response
  std::shared_ptr<Response> response_;

  /// the timeout value provided by user (or the default)
  std::chrono::milliseconds original_timeout_;
  /// the timeout propagated to the downstream service
  std::chrono::milliseconds remote_timeout_;

  DeadlinePropagationConfig deadline_propagation_config_;
  /// deadline from current task
  engine::Deadline deadline_;
  bool timeout_updated_by_deadline_{false};
  bool deadline_expired_{false};

  utils::NotNull<const tracing::TracingManagerBase*> tracing_manager_;
  const server::http::HeadersPropagator* headers_propagator_{nullptr};
  /// struct for reties
  struct {
    /// maximum number of retries
    short retries{1};
    /// current retry
    short current{1};
    /// flag for treating network errors as reason for retry
    bool on_fails{false};
    /// pointer to timer
    std::optional<engine::ev::TimerWatcher> timer;
  } retry_;

  std::optional<tracing::InPlaceSpan> span_storage_;
  std::optional<std::string> log_url_;

  std::atomic<bool> is_cancelled_{false};
  std::array<char, CURL_ERROR_SIZE> errorbuffer_{};

  clients::dns::Resolver* resolver_{nullptr};
  std::string proxy_url_;
  impl::PluginPipeline& plugin_pipeline_;

  struct StreamData {
    StreamData(Queue::Producer&& queue_producer)
        : queue_producer(std::move(queue_producer)) {}

    Queue::Producer queue_producer;
    std::atomic<bool> headers_promise_set{false};
    engine::Promise<void> headers_promise;
  };

  struct FullBufferedData {
    engine::Promise<std::shared_ptr<Response>> promise_;
  };

  std::variant<FullBufferedData, StreamData> data_;
};

}  // namespace clients::http

USERVER_NAMESPACE_END
