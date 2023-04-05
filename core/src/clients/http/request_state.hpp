#pragma once

#include <array>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>
#include <system_error>

#include <userver/clients/dns/resolver_fwd.hpp>
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
#include <clients/http/enforce_task_deadline_config.hpp>
#include <clients/http/testsuite.hpp>
#include <crypto/helpers.hpp>
#include <engine/ev/watcher/timer_watcher.hpp>
#include <server/http/headers_propagator.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

class StreamedResponse;

class RequestState : public std::enable_shared_from_this<RequestState> {
 public:
  RequestState(std::shared_ptr<impl::EasyWrapper>&&,
               std::shared_ptr<RequestStats>&& req_stats,
               const std::shared_ptr<DestinationStatistics>& dest_stats,
               clients::dns::Resolver* resolver,
               impl::PluginPipeline& plugin_pipeline);
  ~RequestState();

  using Queue = concurrent::SpscQueue<std::string>;

  enum class AsyncType {
    kFullyBuffered,
    kStreamed,
  };

  /// Perform async http request
  engine::Future<std::shared_ptr<Response>> async_perform();
  void async_perform_stream(const std::shared_ptr<Queue>& queue);

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
  /// sets proxy to use
  void proxy(const std::string& value);
  /// sets proxy auth type to use
  void proxy_auth_type(curl::easy::proxyauth_t value);

  /// get timeout value in milliseconds
  long timeout() const { return original_timeout_.count(); }
  /// get final timeout value in milliseconds
  long effective_timeout() const { return effective_timeout_.count(); }
  /// get retries count
  short retries() const { return retry_.retries; }

  /// cancel request
  void Cancel();

  void SetDestinationMetricNameAuto(std::string destination);

  void SetDestinationMetricName(const std::string& destination);

  void SetTestsuiteConfig(const std::shared_ptr<const TestsuiteConfig>& config);

  void SetAllowedUrlsExtra(const std::vector<std::string>& urls);

  void DisableReplyDecoding();

  void EnableAddClientTimeoutHeader();
  void DisableAddClientTimeoutHeader();
  void SetEnforceTaskDeadline(EnforceTaskDeadlineConfig enforce_task_deadline);

  std::shared_ptr<impl::EasyWrapper> easy_wrapper() { return easy_; }

  curl::easy& easy() { return easy_->Easy(); }
  const curl::easy& easy() const { return easy_->Easy(); }
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

  /// certifiacte function curl callback
  static curl::native::CURLcode on_certificate_request(void* curl, void* sslctx,
                                                       void* userdata) noexcept;

  /// parse one header
  void parse_header(char* ptr, size_t size);
  void ParseSingleCookie(const char* ptr, size_t size);
  /// simply run perform_request if there is now errors from timer
  void on_retry_timer(std::error_code err);
  /// run curl async_request
  void perform_request(curl::easy::handler_type handler);

  void UpdateTimeoutFromDeadline();
  void UpdateTimeoutHeader();
  std::exception_ptr PrepareDeadlineAlreadyPassedException();

  static size_t StreamWriteFunction(char* ptr, size_t size, size_t nmemb,
                                    void* userdata);

  uint64_t GetClientTimeoutMs() const;
  void UpdateClientTimeoutHeader(uint64_t client_timeout_ms);

  void AccountResponse(std::error_code err);
  std::exception_ptr PrepareException(std::error_code err);
  std::exception_ptr PrepareDeadlinePassedException(std::string_view url);

  engine::Future<std::shared_ptr<Response>> StartNewPromise();
  void ApplyTestsuiteConfig();
  void StartNewSpan();
  void StartStats();

  template <typename Func>
  void WithRequestStats(const Func& func);

  void ResolveTargetAddress(clients::dns::Resolver& resolver);
  void ScheduleWrite();
  bool IsStreamBody() const;

  /// curl handler wrapper
  std::shared_ptr<impl::EasyWrapper> easy_;
  std::shared_ptr<RequestStats> stats_;
  std::shared_ptr<RequestStats> dest_req_stats_;

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
  /// the timeout, possibly updated by deadline propagation
  std::chrono::milliseconds effective_timeout_;

  bool add_client_timeout_header_{true};
  bool report_timeout_as_cancellation_{false};
  EnforceTaskDeadlineConfig enforce_task_deadline_{};
  /// deadline from current task
  engine::Deadline deadline_;
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
        : queue_producer(std::move(queue_producer)),
          headers_promise_set(false),
          headers_future(headers_promise.get_future()) {}

    Queue::Producer queue_producer;
    std::atomic<bool> headers_promise_set;
    engine::Promise<void> headers_promise;
    engine::Future<void> headers_future;
  };

  struct FullBufferedData {
    engine::Promise<std::shared_ptr<Response>> promise_;
  };

  std::variant<FullBufferedData, StreamData> data_;

  friend class StreamedResponse;
};

}  // namespace clients::http

USERVER_NAMESPACE_END
