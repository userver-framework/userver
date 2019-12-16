#include <clients/http/request.hpp>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/system/error_code.hpp>

#include <openssl/ssl.h>

#include <clients/http/destination_statistics.hpp>
#include <clients/http/error.hpp>
#include <clients/http/form.hpp>
#include <clients/http/response_future.hpp>
#include <clients/http/statistics.hpp>
#include <clients/http/testsuite.hpp>
#include <crypto/helpers.hpp>
#include <curl-ev/easy.hpp>
#include <engine/ev/watcher/timer_watcher.hpp>
#include <http/common_headers.hpp>
#include <http/url.hpp>
#include <tracing/span.hpp>
#include <tracing/tags.hpp>

namespace clients {
namespace http {

namespace {
/// Maximum number of redirects
constexpr long kMaxRedirectCount = 10;
/// Max number of retries during calculating timeout
constexpr int kMaxRetryInTimeout = 5;
/// Base time for exponential backoff algorithm
constexpr long kEBBaseTime = 25;
/// Least http code that we treat as bad for exponential backoff algorithm
constexpr long kLeastBadHttpCodeForEB = 500;

const std::string kTracingClientName = "external";

const std::string kHeaderExpect = "Expect";

std::string ToString(HttpMethod method) {
  switch (method) {
    case DELETE:
      return "DELETE";
    case GET:
      return "GET";
    case HEAD:
      return "HEAD";
    case POST:
      return "POST";
    case PUT:
      return "PUT";
    case PATCH:
      return "PATCH";
    case OPTIONS:
      return "OPTIONS";
  }
}

const std::map<std::string, std::error_code> kTestsuiteActions = {
    {"timeout", {curl::errc::easy::operation_timedout}},
    {"network", {curl::errc::easy::could_not_connect}}};
const std::string kTestsuiteSupportedErrors =
    boost::algorithm::join(boost::adaptors::keys(kTestsuiteActions), ",");

[[noreturn]] void AbortWithStacktrace() {
  auto trace = boost::stacktrace::stacktrace();
  std::string trace_msg = boost::stacktrace::to_string(trace);
  std::cerr << "Stacktrace: " << trace_msg << "\n";
  std::abort();
}

std::error_code TestsuiteResponseHook(const Response& response,
                                      tracing::Span& span) {
  if (response.status_code() == 599) {
    const auto& headers = response.headers();
    const auto it = headers.find("X-Testsuite-Error");

    if (headers.end() != it) {
      LOG_INFO() << "Mockserver faked error of type " << it->second << span;

      const auto error_it = kTestsuiteActions.find(it->second);
      if (error_it != kTestsuiteActions.end()) {
        return error_it->second;
      }

      std::cerr
          << "Unsupported mockserver protocol X-Testsuite-Error header value: "
          << it->second
          << ". Try to update submodules and recompile project first. If it "
             "does not help please contact testsuite support team.\n";
      AbortWithStacktrace();
    }
  }
  return {};
}

// Not a strict check, but OK for non-header line check
bool IsHttpStatusLineStart(const char* ptr, size_t size) {
  return (size > 5 && memcmp(ptr, "HTTP/", 5) == 0);
}

}  // namespace

// RequestImpl definition

class Request::RequestImpl
    : public std::enable_shared_from_this<Request::RequestImpl> {
 public:
  RequestImpl(std::shared_ptr<EasyWrapper>,
              std::shared_ptr<RequestStats> req_stats,
              std::shared_ptr<DestinationStatistics> dest_stats);

  /// Perform async http request
  engine::Future<std::shared_ptr<Response>> async_perform();

  /// set redirect flags
  void follow_redirects(bool follow);
  /// set verify flags
  void verify(bool verify);
  /// set file holding one or more certificates to verify the peer with
  void ca_info(const std::string& file_path);
  /// set dir with CA certificates
  void ca_file(const std::string& dir_path);
  /// set CRL-file
  void crl_file(const std::string& file_path);
  /// set private key and certificate from memory
  void client_key_cert(crypto::PrivateKey pkey, crypto::Certificate cert);
  /// Set HTTP version
  void http_version(http_version_t version);
  /// set timeout value
  void set_timeout(long timeout_ms);
  /// set number of retries
  void retry(int retries, bool on_fails);

  /// get timeout value
  long timeout() const { return timeout_ms_; }
  /// get retries count
  short retries() const { return retry_.retries; }

  /// cancel request
  void Cancel();

  void SetDestinationMetricNameAuto(std::string destination);

  void SetDestinationMetricName(const std::string& destination);

  void SetTestsuiteConfig(const std::shared_ptr<const TestsuiteConfig>& config);

  std::shared_ptr<EasyWrapper> easy_wrapper() { return easy_; }

  curl::easy& easy() { return easy_->Easy(); }
  const curl::easy& easy() const { return easy_->Easy(); }
  std::shared_ptr<Response> response() const { return response_; }
  std::shared_ptr<Response> response_move() { return std::move(response_); }

 private:
  /// final callback that calls user callback and set value in promise
  static void on_completed(std::shared_ptr<RequestImpl>,
                           const std::error_code& err);
  /// retry callback
  static void on_retry(std::shared_ptr<RequestImpl>,
                       const std::error_code& err);
  /// header function curl callback
  static size_t on_header(void* ptr, size_t size, size_t nmemb, void* userdata);

  /// certifiacte function curl callback
  static curl::native::CURLcode on_certificate_request(void* curl, void* sslctx,
                                                       void* userdata) noexcept;

  /// parse one header
  void parse_header(char* ptr, size_t size);
  /// simply run perform_request if there is now errors from timer
  void on_retry_timer(const std::error_code& err);
  /// run curl async_request
  void perform_request(curl::easy::handler_type handler);

  void AccountResponse(std::error_code err);

  void ApplyTestsuiteConfig();

 private:
  /// curl handler wrapper
  std::shared_ptr<EasyWrapper> easy_;
  std::shared_ptr<RequestStats> stats_;
  std::shared_ptr<RequestStats> dest_req_stats_;

  std::shared_ptr<DestinationStatistics> dest_stats_;
  std::string destination_metric_name_;

  std::shared_ptr<const TestsuiteConfig> testsuite_config_;

  crypto::PrivateKey pkey_;
  crypto::Certificate cert_;

  /// response
  std::shared_ptr<Response> response_;
  engine::Promise<std::shared_ptr<Response>> promise_;
  /// timeout value
  long timeout_ms_;
  /// struct for reties
  struct {
    /// maximum number of retries
    short retries{1};
    /// current retry
    short current{1};
    /// flag for treating network errors as reason for retry
    bool on_fails{false};
    /// pointer to timer
    std::unique_ptr<engine::ev::TimerWatcher> timer;
  } retry_;

  boost::optional<tracing::Span> span_;
};

// Module functions

inline long max_retry_time(short number) {
  long time_ms = 0;
  for (short int i = 1; i < number; ++i) {
    time_ms += kEBBaseTime * ((1 << std::min(i - 1, kMaxRetryInTimeout)) + 1);
  }
  return time_ms;
}

long complete_timeout(long request_timeout, int retries) {
  return static_cast<long>(request_timeout * 1.1 * retries +
                           max_retry_time(retries));
}

// Request implementation

Request::Request(std::shared_ptr<EasyWrapper> wrapper,
                 std::shared_ptr<RequestStats> req_stats,
                 std::shared_ptr<DestinationStatistics> dest_stats)
    : pimpl_(std::make_shared<Request::RequestImpl>(
          std::move(wrapper), std::move(req_stats), std::move(dest_stats))) {
  LOG_DEBUG() << "Request::Request()";
  // default behavior follow redirects and verify ssl
  pimpl_->follow_redirects(true);
  pimpl_->verify(true);

  if (engine::current_task::ShouldCancel()) {
    throw CancelException(
        "Failed to make HTTP request due to task cancellation");
  }
}

ResponseFuture Request::async_perform() {
  return ResponseFuture(pimpl_->async_perform(),
                        std::chrono::milliseconds(complete_timeout(
                            pimpl_->timeout(), pimpl_->retries())),
                        pimpl_->easy_wrapper());
}

std::shared_ptr<Response> Request::perform() { return async_perform().Get(); }

std::shared_ptr<Request> Request::url(const std::string& _url) {
  pimpl_->SetDestinationMetricNameAuto(::http::ExtractMetaTypeFromUrl(_url));
  easy().set_url(_url.c_str());
  return shared_from_this();
}

std::shared_ptr<Request> Request::timeout(long timeout_ms) {
  pimpl_->set_timeout(timeout_ms);
  return shared_from_this();
}

std::shared_ptr<Request> Request::follow_redirects(bool follow) {
  pimpl_->follow_redirects(follow);
  return shared_from_this();
}

std::shared_ptr<Request> Request::verify(bool verify) {
  pimpl_->verify(verify);
  return shared_from_this();
}

std::shared_ptr<Request> Request::ca_info(const std::string& file_path) {
  pimpl_->ca_info(file_path);
  return shared_from_this();
}

std::shared_ptr<Request> Request::ca_file(const std::string& dir_path) {
  pimpl_->ca_file(dir_path);
  return shared_from_this();
}

std::shared_ptr<Request> Request::crl_file(const std::string& file_path) {
  pimpl_->crl_file(file_path);
  return shared_from_this();
}

std::shared_ptr<Request> Request::client_key_cert(crypto::PrivateKey pkey,
                                                  crypto::Certificate cert) {
  pimpl_->client_key_cert(std::move(pkey), std::move(cert));
  return shared_from_this();
}

std::shared_ptr<Request> Request::http_version(http_version_t version) {
  pimpl_->http_version(version);
  return shared_from_this();
}

std::shared_ptr<Request> Request::retry(int retries, bool on_fails) {
  UASSERT_MSG(retries >= 0, "retires < 0 (" + std::to_string(retries) +
                                "), uninitialized variable?");
  if (retries <= 0) retries = 1;
  pimpl_->retry(retries, on_fails);
  return shared_from_this();
}

std::shared_ptr<Request> Request::data(std::string data) {
  if (!data.empty())
    easy().add_header(kHeaderExpect, "",
                      curl::easy::EmptyHeaderAction::kDoNotSend);
  easy().set_post_fields(std::move(data));
  return shared_from_this();
}

std::shared_ptr<Request> Request::form(const std::shared_ptr<Form>& form) {
  easy().set_http_post(form);
  easy().add_header(kHeaderExpect, "",
                    curl::easy::EmptyHeaderAction::kDoNotSend);
  return shared_from_this();
}

std::shared_ptr<Request> Request::headers(const Headers& headers) {
  for (const auto& header : headers)
    easy().add_header(header.first, header.second);
  return shared_from_this();
}

std::shared_ptr<Request> Request::method(HttpMethod method) {
  switch (method) {
    case DELETE:
    case OPTIONS:
      easy().set_custom_request(ToString(method));
      break;
    case GET:
      easy().set_http_get(true);
      break;
    case HEAD:
      easy().set_no_body(true);
      break;
    // NOTE: set_post makes libcURL to read from stdin if no data is set
    case POST:
    case PUT:
    case PATCH:
      easy().set_custom_request(ToString(method));
      // ensure a body as we should send Content-Length for this method
      if (!easy().has_post_data()) data({});
      break;
  };
  return shared_from_this();
}

std::shared_ptr<Request> Request::get() { return method(GET); }
std::shared_ptr<Request> Request::head() { return method(HEAD); }
std::shared_ptr<Request> Request::post() { return method(POST); }
std::shared_ptr<Request> Request::put() { return method(PUT); }
std::shared_ptr<Request> Request::patch() { return method(PATCH); }
std::shared_ptr<Request> Request::delete_method() { return method(DELETE); }

std::shared_ptr<Request> Request::get(const std::string& url) {
  return get()->url(url);
}

std::shared_ptr<Request> Request::head(const std::string& url) {
  return head()->url(url);
}

std::shared_ptr<Request> Request::post(const std::string& url,
                                       const std::shared_ptr<Form>& form) {
  return this->url(url)->form(form);
}

std::shared_ptr<Request> Request::post(const std::string& url,
                                       std::string data) {
  return this->url(url)->data(std::move(data))->post();
}

std::shared_ptr<Request> Request::put(const std::string& url,
                                      std::string data) {
  return this->url(url)->data(std::move(data))->put();
}

std::shared_ptr<Request> Request::patch(const std::string& url,
                                        std::string data) {
  return this->url(url)->data(std::move(data))->patch();
}

std::shared_ptr<Request> Request::delete_method(const std::string& url) {
  return delete_method()->url(url);
}

curl::easy& Request::easy() { return pimpl_->easy(); }
const curl::easy& Request::easy() const { return pimpl_->easy(); }

std::shared_ptr<Response> Request::response() const {
  return pimpl_->response();
}

void Request::Cancel() const { pimpl_->Cancel(); }

std::shared_ptr<Request> Request::SetDestinationMetricName(
    const std::string& destination) {
  pimpl_->SetDestinationMetricName(destination);
  return shared_from_this();
}

std::shared_ptr<Request> Request::SetTestsuiteConfig(
    const std::shared_ptr<const TestsuiteConfig>& config) {
  pimpl_->SetTestsuiteConfig(config);
  return shared_from_this();
}

void Request::RequestImpl::SetDestinationMetricNameAuto(
    std::string destination) {
  destination_metric_name_ = std::move(destination);
}

// RequestImpl implementation

Request::RequestImpl::RequestImpl(
    std::shared_ptr<EasyWrapper> wrapper,
    std::shared_ptr<RequestStats> req_stats,
    std::shared_ptr<DestinationStatistics> dest_stats)
    : easy_(std::move(wrapper)),
      stats_(std::move(req_stats)),
      dest_stats_(std::move(dest_stats)),
      timeout_ms_(0) {
  // Libcurl calls sigaction(2)  way too frequently unless this option is used.
  easy().set_no_signal(true);
}

void Request::RequestImpl::follow_redirects(bool follow) {
  easy().set_follow_location(follow);
  easy().set_post_redir(static_cast<long>(follow));
  if (follow) easy().set_max_redirs(kMaxRedirectCount);
}

void Request::RequestImpl::verify(bool verify) {
  easy().set_ssl_verify_host(verify);
  easy().set_ssl_verify_peer(verify);
}

void Request::RequestImpl::ca_info(const std::string& file_path) {
  easy().set_ca_info(file_path.c_str());
}

void Request::RequestImpl::ca_file(const std::string& dir_path) {
  easy().set_ca_file(dir_path.c_str());
}

void Request::RequestImpl::crl_file(const std::string& file_path) {
  easy().set_crl_file(file_path.c_str());
}

void Request::RequestImpl::client_key_cert(crypto::PrivateKey pkey,
                                           crypto::Certificate cert) {
  YTX_INVARIANT(pkey, "No private key");
  YTX_INVARIANT(cert, "No certificate");

  pkey_ = std::move(pkey);
  cert_ = std::move(cert);
  easy().set_ssl_ctx_function(&Request::RequestImpl::on_certificate_request);
  easy().set_ssl_ctx_data(this);
}

void Request::RequestImpl::http_version(http_version_t version) {
  LOG_DEBUG() << "http_version";
  easy().set_http_version(version);
  LOG_DEBUG() << "http_version after";
}

void Request::RequestImpl::set_timeout(long timeout_ms) {
  UASSERT_MSG(timeout_ms >= 0, "timeout_ms < 0 (" + std::to_string(timeout_ms) +
                                   "), uninitialized variable?");
  timeout_ms_ = timeout_ms;
  easy().set_timeout_ms(timeout_ms);
  easy().set_connect_timeout_ms(timeout_ms);
}

void Request::RequestImpl::retry(int retries, bool on_fails) {
  retry_.retries = retries;
  retry_.current = 1;
  retry_.on_fails = on_fails;
}

void Request::RequestImpl::Cancel() { easy().cancel(); }

void Request::RequestImpl::SetDestinationMetricName(
    const std::string& destination) {
  dest_req_stats_ = dest_stats_->GetStatisticsForDestination(destination);
}

void Request::RequestImpl::SetTestsuiteConfig(
    const std::shared_ptr<const TestsuiteConfig>& config) {
  testsuite_config_ = config;
}

size_t Request::RequestImpl::on_header(void* ptr, size_t size, size_t nmemb,
                                       void* userdata) {
  auto* self = static_cast<Request::RequestImpl*>(userdata);
  size_t data_size = size * nmemb;
  if (self) self->parse_header(static_cast<char*>(ptr), data_size);
  return data_size;
}

curl::native::CURLcode Request::RequestImpl::on_certificate_request(
    void* /*curl*/, void* sslctx, void* userdata) noexcept {
  const auto ssl = static_cast<SSL_CTX*>(sslctx);
  auto* self = static_cast<Request::RequestImpl*>(userdata);

  if (!self) {
    return curl::native::CURLcode::CURLE_ABORTED_BY_CALLBACK;
  }

  if (self->cert_) {
    if (::SSL_CTX_use_certificate(ssl, self->cert_.GetNative()) != 1) {
      return curl::native::CURLcode::CURLE_SSL_CERTPROBLEM;
    }
  }

  if (self->pkey_) {
    if (::SSL_CTX_use_PrivateKey(ssl, self->pkey_.GetNative()) != 1) {
      return curl::native::CURLcode::CURLE_SSL_CERTPROBLEM;
    }
  }

  return curl::native::CURLcode::CURLE_OK;
}

void Request::RequestImpl::on_completed(
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    std::shared_ptr<Request::RequestImpl> holder,
    const std::error_code& orig_err) {
  auto err = orig_err;
  auto& span = *holder->span_;
  LOG_DEBUG() << "Request::RequestImpl::on_completed(1)" << span;

  if (holder->testsuite_config_) {
    if (!err) err = TestsuiteResponseHook(*holder->response(), span);
  }

  holder->AccountResponse(err);

  span.AddTag(tracing::kAttempts, holder->retry_.current);
  span.AddTag(tracing::kMaxAttempts, holder->retry_.retries);
  span.AddTag(tracing::kTimeoutMs, holder->timeout_ms_);

  LOG_DEBUG() << "Request::RequestImpl::on_completed(2)" << span;
  if (err) {
    span.AddTag(tracing::kErrorFlag, true);
    span.AddTag(tracing::kErrorMessage, err.message());
    span.AddTag(tracing::kHttpStatusCode, 599);  // TODO

    holder->promise_.set_exception(PrepareException(err));
  } else {
    span.AddTag(tracing::kHttpStatusCode, holder->response()->status_code());
    if (!holder->response()->IsOk()) span.AddTag(tracing::kErrorFlag, true);

    holder->promise_.set_value(holder->response_move());
  }

  LOG_DEBUG() << "Request::RequestImpl::on_completed(3)" << span;
  holder->span_.reset();
}

void Request::RequestImpl::AccountResponse(std::error_code err) {
  const auto attempts = retry_.current;

  stats_->StoreTimeToStart(easy().timings().time_to_start());
  if (err)
    stats_->FinishEc(err, attempts);
  else
    stats_->FinishOk(easy().get_response_code(), attempts);

  if (dest_req_stats_) {
    dest_req_stats_->StoreTimeToStart(easy().timings().time_to_start());
    if (err)
      dest_req_stats_->FinishEc(err, attempts);
    else
      dest_req_stats_->FinishOk(easy().get_response_code(), attempts);
  }
}

void Request::RequestImpl::on_retry(
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    std::shared_ptr<Request::RequestImpl> holder, const std::error_code& err) {
  LOG_DEBUG() << "RequestImpl::on_retry" << *holder->span_;

  // We do not need to retry
  //  - if we got result and http code is good
  //  - if we use all tries
  //  - if error and we should not retry on error
  bool not_need_retry =
      (!err && holder->easy().get_response_code() < kLeastBadHttpCodeForEB) ||
      (holder->retry_.current >= holder->retry_.retries) ||
      (err && !holder->retry_.on_fails);
  if (not_need_retry) {
    // finish if don't need retry
    holder->on_completed(holder, err);
  } else {
    holder->AccountResponse(err);

    // calculate timeout before retry
    long timeout_ms =
        kEBBaseTime * (rand() % ((1 << std::min(holder->retry_.current - 1,
                                                kMaxRetryInTimeout))) +
                       1);
    // increase try
    ++holder->retry_.current;
    // initialize timer
    holder->retry_.timer = std::make_unique<engine::ev::TimerWatcher>(
        holder->easy().GetThreadControl());
    // call on_retry_timer on timer
    holder->retry_.timer->SingleshotAsync(
        std::chrono::milliseconds(timeout_ms),
        std::bind(&Request::RequestImpl::on_retry_timer, holder,
                  std::placeholders::_1));
  }
}

void Request::RequestImpl::on_retry_timer(const std::error_code& err) {
  // if there is no error with timer call perform, otherwise finish
  if (!err)
    perform_request(std::bind(&Request::RequestImpl::on_retry,
                              shared_from_this(), std::placeholders::_1));
  else
    on_completed(shared_from_this(), err);
}

char* rfind_not_space(char* ptr, size_t size) {
  for (char* p = ptr + size - 1; p >= ptr; --p) {
    char c = *p;
    if (c == '\n' || c == '\r' || c == ' ' || c == '\t') continue;
    return p + 1;
  }
  return ptr;
}

void Request::RequestImpl::parse_header(char* ptr, size_t size) {
  /* It is a fast path in curl's thread (io thread).  Creation of tmp
   * std::string, boost::trim_right_if(), etc. is too expensive. */
  auto end = rfind_not_space(ptr, size);
  if (ptr == end) return;
  *end = '\0';

  const char* col_pos = static_cast<const char*>(memchr(ptr, ':', size));
  if (col_pos == nullptr) {
    if (IsHttpStatusLineStart(ptr, size)) {
      for (auto& [k, v] : response_->headers())
        LOG_INFO() << "drop header " << k << "=" << v;
      // In case of redirect drop 1st response headers
      response_->headers().clear();
    }
    return;
  }

  std::string key(ptr, col_pos - ptr);

  ++col_pos;

  // From https://tools.ietf.org/html/rfc7230#page-22 :
  //
  // header-field   = field-name ":" OWS field-value OWS
  // OWS            = *( SP / HTAB )
  while (end != col_pos && (*col_pos == ' ' || *col_pos == '\t')) {
    ++col_pos;
  }

  std::string value(col_pos, end - col_pos);
  response_->headers().emplace(std::move(key), std::move(value));
}

engine::Future<std::shared_ptr<Response>>
Request::RequestImpl::async_perform() {
  span_.emplace(kTracingClientName);
  easy().add_header(::http::headers::kXYaSpanId, span_->GetSpanId());
  easy().add_header(::http::headers::kXYaTraceId, span_->GetTraceId());
  easy().add_header(::http::headers::kXYaRequestId, span_->GetLink());
  span_->AddTag(tracing::kHttpUrl, easy().get_effective_url());

  ApplyTestsuiteConfig();

  // Span is local to a Request, it is not related to current coroutine
  span_->DetachFromCoroStack();

  // define header function
  easy().set_header_function(&Request::RequestImpl::on_header);
  easy().set_header_data(this);

  // set autodecooding for gzip and deflate
  easy().set_accept_encoding("gzip,deflate");

  if (!dest_req_stats_) {
    dest_req_stats_ =
        dest_stats_->GetStatisticsForDestinationAuto(destination_metric_name_);
  }

  stats_->Start();
  if (dest_req_stats_) dest_req_stats_->Start();

  // if we need retries call with special callback
  if (retry_.retries <= 1)
    perform_request(std::bind(&Request::RequestImpl::on_completed,
                              shared_from_this(), std::placeholders::_1));
  else
    perform_request(std::bind(&Request::RequestImpl::on_retry,
                              shared_from_this(), std::placeholders::_1));

  return promise_.get_future();
}

void Request::RequestImpl::perform_request(curl::easy::handler_type handler) {
  UASSERT_MSG(!cert_ || pkey_,
              "Setting certificate is useless without setting private key");

  response_ = std::make_shared<Response>(easy_);
  // set place for response body
  easy().set_sink(&(response_->sink_stream()));

  // perform request
  easy().async_perform(std::move(handler));
}

void Request::RequestImpl::ApplyTestsuiteConfig() {
  if (!testsuite_config_) {
    return;
  }

  const auto& prefixes = testsuite_config_->allowed_url_prefixes;
  if (!prefixes.empty()) {
    std::string url = easy().get_effective_url();
    if (std::find_if(prefixes.begin(), prefixes.end(),
                     [&url](const std::string& prefix) {
                       return boost::starts_with(url, prefix);
                     }) == prefixes.end()) {
      std::cerr << url << " forbidden by testsuite config, allowed prefixes=\n"
                << boost::algorithm::join(prefixes, "\n") << "\n";
      AbortWithStacktrace();
    }
  }

  const auto& timeout = testsuite_config_->http_request_timeout;
  if (timeout) {
    set_timeout(std::chrono::milliseconds(*timeout).count());
  }

  easy().add_header("X-Testsuite-Supported-Errors", kTestsuiteSupportedErrors);
}

}  // namespace http
}  // namespace clients
