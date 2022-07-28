#include <clients/http/request_state.hpp>

#include <algorithm>
#include <chrono>
#include <map>
#include <string_view>

#include <fmt/format.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/map.hpp>

#include <userver/clients/dns/resolver.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/encoding/hex.hpp>
#include <userver/utils/from_string.hpp>
#include <userver/utils/rand.hpp>
#include <utils/impl/assert_extra.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

namespace {
/// Default timeout
constexpr auto kDefaultTimeout = std::chrono::milliseconds{100};
/// Maximum number of redirects
constexpr long kMaxRedirectCount = 10;
/// Max power value for exponential backoff algorithm
constexpr int kEBMaxPower = 5;
/// Base time for exponential backoff algorithm
constexpr auto kEBBaseTime = std::chrono::milliseconds{25};
/// Least http code that we treat as bad for exponential backoff algorithm
constexpr long kLeastBadHttpCodeForEB = 500;

constexpr Status kFakeHttpErrorCode{599};

const std::string kTracingClientName = "external";

const std::map<std::string, std::error_code> kTestsuiteActions = {
    {"timeout", {curl::errc::EasyErrorCode::kOperationTimedout}},
    {"network", {curl::errc::EasyErrorCode::kCouldNotConnect}}};
const std::string kTestsuiteSupportedErrorsKey = "X-Testsuite-Supported-Errors";
const std::string kTestsuiteSupportedErrors =
    boost::algorithm::join(boost::adaptors::keys(kTestsuiteActions), ",");

std::error_code TestsuiteResponseHook(Status status_code,
                                      const Headers& headers,
                                      tracing::Span& span) {
  if (status_code == kFakeHttpErrorCode) {
    const auto it = headers.find("X-Testsuite-Error");

    if (headers.end() != it) {
      LOG_INFO() << "Mockserver faked error of type " << it->second << span;

      const auto error_it = kTestsuiteActions.find(it->second);
      if (error_it != kTestsuiteActions.end()) {
        return error_it->second;
      }

      utils::impl::AbortWithStacktrace(fmt::format(
          "Unsupported mockserver protocol X-Testsuite-Error header value: {}. "
          "Try to update submodules and recompile project first. If it does "
          "not help please contact testsuite support team.",
          it->second));
    }
  }
  return {};
}

// Not a strict check, but OK for non-header line check
bool IsHttpStatusLineStart(const char* ptr, size_t size) {
  return (size > 5 && memcmp(ptr, "HTTP/", 5) == 0);
}

char* rfind_not_space(char* ptr, size_t size) {
  for (char* p = ptr + size - 1; p >= ptr; --p) {
    char c = *p;
    if (c == '\n' || c == '\r' || c == ' ' || c == '\t') continue;
    return p + 1;
  }
  return ptr;
}

engine::Deadline GetTaskDeadline() {
  const auto* const data = server::request::kTaskInheritedData.GetOptional();
  return data ? data->deadline : engine::Deadline{};
}

void SetTracingHeader(curl::easy& e, std::string_view name,
                      std::string_view value) {
  e.add_header(name, value, curl::easy::EmptyHeaderAction::kDoNotSend,
               curl::easy::DuplicateHeaderAction::kReplace);
}

bool IsTimeout(std::error_code ec) noexcept {
  return ec ==
         std::error_code(
             static_cast<int>(curl::errc::EasyErrorCode::kOperationTimedout),
             curl::errc::GetEasyCategory());
}

}  // namespace

void RequestState::SetDestinationMetricNameAuto(std::string destination) {
  destination_metric_name_ = std::move(destination);
}

RequestState::RequestState(
    std::shared_ptr<impl::EasyWrapper>&& wrapper,
    std::shared_ptr<RequestStats>&& req_stats,
    const std::shared_ptr<DestinationStatistics>& dest_stats,
    clients::dns::Resolver* resolver)
    : easy_(std::move(wrapper)),
      stats_(std::move(req_stats)),
      dest_stats_(dest_stats),
      timeout_(std::chrono::duration_cast<std::chrono::milliseconds>(
          kDefaultTimeout)),
      deadline_(GetTaskDeadline()),
      is_cancelled_(false),
      errorbuffer_(),
      resolver_{resolver} {
  // Libcurl calls sigaction(2)  way too frequently unless this option is used.
  easy().set_no_signal(true);
  easy().set_error_buffer(errorbuffer_.data());

  // define header function
  easy().set_header_function(&RequestState::on_header);
  easy().set_header_data(this);

  // set autodecoding for gzip and deflate
  easy().set_accept_encoding("gzip,deflate");
}

RequestState::~RequestState() {
  std::error_code ec;
  easy().set_error_buffer(nullptr, ec);
  UASSERT(!ec);
}

void RequestState::follow_redirects(bool follow) {
  easy().set_follow_location(follow);
  easy().set_post_redir(static_cast<long>(follow));
  if (follow) easy().set_max_redirs(kMaxRedirectCount);
}

void RequestState::verify(bool verify) {
  easy().set_ssl_verify_host(verify);
  easy().set_ssl_verify_peer(verify);
}

void RequestState::ca_info(const std::string& file_path) {
  easy().set_ca_info(file_path.c_str());
}

void RequestState::ca(crypto::Certificate cert) {
  ca_ = std::move(cert);
  easy().set_ssl_ctx_function(&RequestState::on_certificate_request);
  easy().set_ssl_ctx_data(this);
}

void RequestState::crl_file(const std::string& file_path) {
  easy().set_crl_file(file_path.c_str());
}

void RequestState::client_key_cert(crypto::PrivateKey pkey,
                                   crypto::Certificate cert) {
  UINVARIANT(pkey, "No private key");
  UINVARIANT(cert, "No certificate");

  pkey_ = std::move(pkey);
  cert_ = std::move(cert);

  // FIXME: until cURL 7.71 there is no sane way to pass TLS keys from memory.
  // Because of this, we provide our own callback. As a consequence, cURL has
  // no knowledge of the key used and may reuse this connection for a request
  // with a different key or without one.
  // To avoid this until we can upgrade we set the EGD socket option to
  // an unusable certificate-specific value. This option should have no effect
  // on systems targeted by userver anyway but it is accounted when checking
  // cached connection eligibility which is exactly what we need.

  // must be larger than sizeof(sockaddr_un::sun_path)
  static constexpr size_t kCertIdLength = 255;

  // backwards incompatibility
#if OPENSSL_VERSION_NUMBER >= 0x010100000L
  const
#endif
      ASN1_BIT_STRING* cert_sig = nullptr;
  X509_get0_signature(&cert_sig, nullptr, cert_.GetNative());
  UINVARIANT(cert_sig, "Cannot get X509 certificate signature");

  std::string cert_id;
  cert_id.reserve(kCertIdLength);
  utils::encoding::ToHex(
      std::string_view{reinterpret_cast<const char*>(cert_sig->data),
                       std::min<size_t>(cert_sig->length, kCertIdLength / 2)},
      cert_id);
  cert_id.resize(kCertIdLength, '=');
  easy().set_egd_socket(cert_id);

  easy().set_ssl_ctx_function(&RequestState::on_certificate_request);
  easy().set_ssl_ctx_data(this);
}

void RequestState::http_version(curl::easy::http_version_t version) {
  easy().set_http_version(version);
}

void RequestState::set_timeout(long timeout_ms) {
  UASSERT_MSG(timeout_ms >= 0, "timeout_ms < 0 (" + std::to_string(timeout_ms) +
                                   "), uninitialized variable?");
  timeout_ = std::chrono::milliseconds{timeout_ms};
  easy().set_timeout_ms(timeout_ms);
  easy().set_connect_timeout_ms(timeout_ms);
}

void RequestState::retry(short retries, bool on_fails) {
  retry_.retries = retries;
  retry_.current = 1;
  retry_.on_fails = on_fails;
}

void RequestState::unix_socket_path(const std::string& path) {
  easy().set_unix_socket_path(path);
}

void RequestState::proxy(const std::string& value) {
  proxy_url_ = value;
  easy().set_proxy(value);
}

void RequestState::proxy_auth_type(curl::easy::proxyauth_t value) {
  easy().set_proxy_auth(value);
}

void RequestState::Cancel() {
  // We can not call `retry_.timer.reset();` here because of data race
  is_cancelled_ = true;
  easy().cancel();
}

void RequestState::SetDestinationMetricName(const std::string& destination) {
  dest_req_stats_ = dest_stats_->GetStatisticsForDestination(destination);
}

void RequestState::SetTestsuiteConfig(
    const std::shared_ptr<const TestsuiteConfig>& config) {
  testsuite_config_ = config;
}

void RequestState::DisableReplyDecoding() {
  easy().set_accept_encoding(nullptr);
}

void RequestState::EnableAddClientTimeoutHeader() {
  add_client_timeout_header_ = true;
}

void RequestState::DisableAddClientTimeoutHeader() {
  add_client_timeout_header_ = false;
}

void RequestState::SetEnforceTaskDeadline(
    EnforceTaskDeadlineConfig enforce_task_deadline) {
  enforce_task_deadline_ = enforce_task_deadline;
}

size_t RequestState::on_header(void* ptr, size_t size, size_t nmemb,
                               void* userdata) {
  auto* self = static_cast<RequestState*>(userdata);
  size_t data_size = size * nmemb;
  if (self) self->parse_header(static_cast<char*>(ptr), data_size);
  return data_size;
}

curl::native::CURLcode RequestState::on_certificate_request(
    void* /*curl*/, void* sslctx, void* userdata) noexcept {
  auto* ssl = static_cast<SSL_CTX*>(sslctx);
  auto* self = static_cast<RequestState*>(userdata);

  if (!self) {
    return curl::native::CURLcode::CURLE_ABORTED_BY_CALLBACK;
  }

  if (self->ca_) {
    auto* store = SSL_CTX_get_cert_store(ssl);
    if (1 != X509_STORE_add_cert(store, self->ca_.GetNative())) {
      LOG_ERROR() << crypto::FormatSslError(
          "Failed to set up server TLS wrapper: X509_STORE_add_cert");
      return curl::native::CURLcode::CURLE_SSL_CERTPROBLEM;
    }
  }

  if (self->cert_) {
    if (::SSL_CTX_use_certificate(ssl, self->cert_.GetNative()) != 1) {
      LOG_ERROR() << crypto::FormatSslError(
          "Failed to set up server TLS wrapper: SSL_CTX_use_certificate");
      return curl::native::CURLcode::CURLE_SSL_CERTPROBLEM;
    }
  }

  if (self->pkey_) {
    if (::SSL_CTX_use_PrivateKey(ssl, self->pkey_.GetNative()) != 1) {
      LOG_ERROR() << crypto::FormatSslError(
          "Failed to set up server TLS wrapper: SSL_CTX_use_PrivateKey");
      return curl::native::CURLcode::CURLE_SSL_CERTPROBLEM;
    }
  }

  return curl::native::CURLcode::CURLE_OK;
}

void RequestState::on_completed(std::shared_ptr<RequestState> holder,
                                std::error_code err) {
  UASSERT(holder);
  UASSERT(holder->span_storage_);
  auto& span = holder->span_storage_->Get();
  auto& easy = holder->easy();
  LOG_TRACE() << "Request::RequestImpl::on_completed(1)" << span;
  const auto status_code = static_cast<Status>(easy.get_response_code());

  if (holder->testsuite_config_ && !err) {
    const auto& headers = holder->response()->headers();
    err = TestsuiteResponseHook(status_code, headers, span);
  }

  holder->AccountResponse(err);
  const auto sockets = easy.get_num_connects();
  holder->WithRequestStats(
      [sockets](RequestStats& stats) { stats.AccountOpenSockets(sockets); });

  span.AddTag(tracing::kAttempts, holder->retry_.current);
  span.AddTag(tracing::kMaxAttempts, holder->retry_.retries);
  span.AddTag(tracing::kTimeoutMs, holder->timeout_.count());

  LOG_TRACE() << "Request::RequestImpl::on_completed(2)" << span;
  if (err) {
    if (easy.rate_limit_error()) {
      // The most probable cause, takes precedence
      err = easy.rate_limit_error();
    }

    span.AddTag(tracing::kErrorFlag, true);
    span.AddTag(tracing::kErrorMessage, err.message());
    span.AddTag(tracing::kHttpStatusCode, kFakeHttpErrorCode);

    if (holder->errorbuffer_.front()) {
      LOG_DEBUG() << "cURL error details: " << holder->errorbuffer_.data();
    }

    const auto cleanup_request = holder->response_move();
    holder->span_storage_.reset();
    holder->promise_.set_exception(holder->PrepareException(err));
  } else {
    span.AddTag(tracing::kHttpStatusCode, status_code);
    holder->response()->SetStatusCode(status_code);
    holder->response()->SetStats(easy.get_local_stats());

    if (!holder->response()->IsOk()) span.AddTag(tracing::kErrorFlag, true);

    holder->span_storage_.reset();
    holder->promise_.set_value(holder->response_move());
  }

  // it is unsafe to touch any content of holder after this point!

  LOG_TRACE() << "Request::RequestImpl::on_completed(3)";
}

void RequestState::on_retry(std::shared_ptr<RequestState> holder,
                            std::error_code err) {
  UASSERT(holder);
  UASSERT(holder->span_storage_);
  LOG_TRACE() << "RequestImpl::on_retry" << holder->span_storage_->Get();

  // We do not need to retry
  //  - if we got result and http code is good
  //  - if we use all tries
  //  - if error and we should not retry on error
  bool not_need_retry =
      (!err && holder->easy().get_response_code() < kLeastBadHttpCodeForEB) ||
      (holder->retry_.current >= holder->retry_.retries) ||
      (err && !holder->retry_.on_fails) || holder->is_cancelled_.load();
  if (not_need_retry) {
    // finish if don't need retry
    holder->on_completed(holder, err);
  } else {
    holder->AccountResponse(err);

    // calculate backoff before retry
    const auto eb_power =
        std::clamp(holder->retry_.current - 1, 0, kEBMaxPower);
    auto backoff = kEBBaseTime * (utils::RandRange(1 << eb_power) + 1);
    // increase try
    ++holder->retry_.current;
    holder->easy().mark_retry();

    holder->retry_.timer.emplace(holder->easy().GetThreadControl());

    // call on_retry_timer on timer
    holder->retry_.timer->SingleshotAsync(
        backoff, [holder = std::move(holder)](std::error_code err) {
          holder->on_retry_timer(err);
        });
  }
}

void RequestState::on_retry_timer(std::error_code err) {
  // if there is no error with timer call perform, otherwise finish
  if (!err)
    perform_request([holder = shared_from_this()](std::error_code err) mutable {
      RequestState::on_retry(std::move(holder), err);
    });
  else
    on_completed(shared_from_this(), err);
}

void RequestState::parse_header(char* ptr, size_t size) {
  /* It is a fast path in curl's thread (io thread).  Creation of tmp
   * std::string, boost::trim_right_if(), etc. is too expensive. */
  auto* end = rfind_not_space(ptr, size);
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

void RequestState::SetLoggedUrl(std::string url) { log_url_ = std::move(url); }

engine::Future<std::shared_ptr<Response>> RequestState::async_perform() {
  StartNewSpan();

  auto future = StartNewPromise();
  ApplyTestsuiteConfig();
  StartStats();

  // if we need retries call with special callback
  if (retry_.retries <= 1) {
    perform_request([holder = shared_from_this()](std::error_code err) mutable {
      RequestState::on_completed(std::move(holder), err);
    });
  } else {
    perform_request([holder = shared_from_this()](std::error_code err) mutable {
      RequestState::on_retry(std::move(holder), err);
    });
  }

  return future;
}

void RequestState::perform_request(curl::easy::handler_type handler) {
  UASSERT_MSG(!cert_ || pkey_,
              "Setting certificate is useless without setting private key");

  UASSERT(response_);
  response_->sink_string().clear();
  response_->headers().clear();

  UpdateTimeoutFromDeadline();
  if (timeout_ <= std::chrono::milliseconds{0}) {
    promise_.set_exception(PrepareDeadlineAlreadyPassedException());
    return;
  }
  UpdateTimeoutHeader();

  if (resolver_ && retry_.current == 1) {
    engine::AsyncNoSpan([this, holder = shared_from_this(),
                         handler = std::move(handler)]() mutable {
      try {
        ResolveTargetAddress(*resolver_);
        easy().async_perform(std::move(handler));
      } catch (const clients::dns::ResolverException& ex) {
        // TODO: should retry - TAXICOMMON-4932
        promise_.set_exception(std::make_exception_ptr(ex));
      } catch (const BaseException& ex) {
        promise_.set_exception(std::make_exception_ptr(ex));
      }
    }).Detach();
  } else {
    easy().async_perform(std::move(handler));
  }
}

void RequestState::UpdateTimeoutFromDeadline() {
  UASSERT(timeout_ >= std::chrono::milliseconds{0});
  report_timeout_as_cancellation_ = false;

  if (enforce_task_deadline_.update_timeout && deadline_.IsReachable()) {
    // TODO: account socket rtt. https://st.yandex-team.ru/TAXICOMMON-3506
    const auto timeout_from_deadline =
        std::max(std::chrono::duration_cast<std::chrono::milliseconds>(
                     deadline_.TimeLeft()),
                 std::chrono::milliseconds{0});

    if (timeout_from_deadline < timeout_) {
      set_timeout(timeout_from_deadline.count());
      if (enforce_task_deadline_.cancel_request) {
        report_timeout_as_cancellation_ = true;
      }
      WithRequestStats(
          [](RequestStats& stats) { stats.AccountTimeoutUpdatedByDeadline(); });
    }
  }
}

void RequestState::UpdateTimeoutHeader() {
  if (!add_client_timeout_header_) return;

  const auto old_timeout_str = easy().FindHeaderByName(
      USERVER_NAMESPACE::http::headers::kXYaTaxiClientTimeoutMs);
  if (old_timeout_str) {
    try {
      const std::chrono::milliseconds old_timeout{
          utils::FromString<std::chrono::milliseconds::rep>(
              std::string{*old_timeout_str})};
      if (old_timeout <= timeout_) return;
    } catch (const std::exception& ex) {
      LOG_LIMITED_WARNING()
          << "Can't parse client_timeout_ms from '" << *old_timeout_str << '\'';
    }
  }
  easy().add_header(USERVER_NAMESPACE::http::headers::kXYaTaxiClientTimeoutMs,
                    fmt::to_string(timeout_.count()),
                    old_timeout_str
                        ? curl::easy::DuplicateHeaderAction::kReplace
                        : curl::easy::DuplicateHeaderAction::kAdd);
}

std::exception_ptr RequestState::PrepareDeadlineAlreadyPassedException() {
  const auto& url = easy().get_original_url();  // no effective_url yet

  if (enforce_task_deadline_.cancel_request) {
    return PrepareDeadlinePassedException(url);
  } else {
    return std::make_exception_ptr(
        TimeoutException(fmt::format("Timeout happened, url: {}", url),
                         easy().get_local_stats()));
  }
}

void RequestState::AccountResponse(std::error_code err) {
  const auto attempts = retry_.current;

  const auto time_to_start =
      std::chrono::duration_cast<std::chrono::microseconds>(
          easy().time_to_start());

  WithRequestStats([&](RequestStats& stats) {
    stats.StoreTimeToStart(time_to_start);
    if (err)
      stats.FinishEc(err, attempts);
    else
      stats.FinishOk(static_cast<int>(easy().get_response_code()), attempts);
  });
}

std::exception_ptr RequestState::PrepareException(std::error_code err) {
  if (report_timeout_as_cancellation_ && IsTimeout(err)) {
    return PrepareDeadlinePassedException(easy().get_effective_url());
  }

  return http::PrepareException(err, easy().get_effective_url(),
                                easy().get_local_stats());
}

std::exception_ptr RequestState::PrepareDeadlinePassedException(
    std::string_view url) {
  WithRequestStats(
      [](RequestStats& stats) { stats.AccountCancelledByDeadline(); });

  return std::make_exception_ptr(CancelException(
      fmt::format("Timeout happened (deadline propagation), url: {}", url),
      easy().get_local_stats()));
}

engine::Future<std::shared_ptr<Response>> RequestState::StartNewPromise() {
  promise_ = {};

  response_ = std::make_shared<Response>();
  easy().set_sink(&(response_->sink_string()));  // set place for response body

  is_cancelled_ = false;
  retry_.current = 1;

  return promise_.get_future();
}

void RequestState::ApplyTestsuiteConfig() {
  if (!testsuite_config_) {
    return;
  }

  const auto& prefixes = testsuite_config_->allowed_url_prefixes;
  if (!prefixes.empty()) {
    auto url = easy().get_original_url();
    if (std::find_if(prefixes.begin(), prefixes.end(),
                     [&url](const std::string& prefix) {
                       return boost::starts_with(url, prefix);
                     }) == prefixes.end()) {
      utils::impl::AbortWithStacktrace(
          fmt::format("{} forbidden by testsuite config, allowed prefixes={}",
                      url, fmt::join(prefixes, ",")));
    }
  }

  const auto& timeout = testsuite_config_->http_request_timeout;
  if (timeout) {
    set_timeout(std::chrono::milliseconds(*timeout).count());
  }

  easy().add_header(kTestsuiteSupportedErrorsKey, kTestsuiteSupportedErrors);
}

void RequestState::StartNewSpan() {
  UINVARIANT(
      !span_storage_,
      "Attempt to reuse request while the previous one has not finished");

  span_storage_.emplace(std::string{kTracingClientName});
  auto& span = span_storage_->Get();
  SetTracingHeader(easy(), USERVER_NAMESPACE::http::headers::kXYaSpanId,
                   span.GetSpanId());
  SetTracingHeader(easy(), USERVER_NAMESPACE::http::headers::kXYaTraceId,
                   span.GetTraceId());
  SetTracingHeader(easy(), USERVER_NAMESPACE::http::headers::kXYaRequestId,
                   span.GetLink());

  // effective url is not available yet
  span.AddTag(tracing::kHttpUrl,
              log_url_ ? *log_url_ : easy().get_original_url());

  // Span is local to a Request, it is not related to current coroutine
  span.DetachFromCoroStack();
}

void RequestState::StartStats() {
  if (!dest_req_stats_) {
    dest_req_stats_ =
        dest_stats_->GetStatisticsForDestinationAuto(destination_metric_name_);
  }

  WithRequestStats([](RequestStats& stats) { stats.Start(); });
}

template <typename Func>
void RequestState::WithRequestStats(const Func& func) {
  static_assert(std::is_invocable_v<const Func&, RequestStats&>);
  func(*stats_);
  if (dest_req_stats_) func(*dest_req_stats_);
}

void RequestState::ResolveTargetAddress(clients::dns::Resolver& resolver) {
  auto deadline = timeout_.count() > 0
                      ? engine::Deadline::FromDuration(timeout_)
                      : engine::Deadline{};
  auto target = curl::url{};
  if (!proxy_url_.empty()) {
    std::error_code ec;
    target.SetDefaultSchemeUrl(proxy_url_.c_str(), ec);
    if (ec) throw BadArgumentException(ec, "Bad proxy URL", proxy_url_, {});
  } else {
    target.SetUrl(easy().get_original_url().c_str());
  }

  std::vector<std::string> addrs;
  for (const auto& addr :
       resolver.Resolve(target.GetHostPtr().get(), deadline)) {
    addrs.push_back(addr.PrimaryAddressString());
  }
  easy().add_resolve(target.GetHostPtr().get(), target.GetPortPtr().get(),
                     fmt::to_string(fmt::join(addrs, ",")));
}

}  // namespace clients::http

USERVER_NAMESPACE_END
