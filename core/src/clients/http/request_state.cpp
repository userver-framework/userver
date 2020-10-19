#include <clients/http/request_state.hpp>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <system_error>

#include <openssl/ssl.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/stacktrace/stacktrace.hpp>
#include <boost/system/error_code.hpp>

#include <clients/http/destination_statistics.hpp>
#include <clients/http/error.hpp>
#include <clients/http/form.hpp>
#include <clients/http/response_future.hpp>
#include <clients/http/statistics.hpp>
#include <http/common_headers.hpp>
#include <http/url.hpp>
#include <tracing/span.hpp>
#include <tracing/tags.hpp>

#include <clients/http/easy_wrapper.hpp>
#include <clients/http/testsuite.hpp>
#include <crypto/helpers.hpp>
#include <engine/blocking_future.hpp>
#include <engine/ev/watcher/timer_watcher.hpp>

namespace clients::http {

namespace {
/// Default timeout
constexpr auto kDefaultTimeout = std::chrono::milliseconds{100};
/// Maximum number of redirects
constexpr long kMaxRedirectCount = 10;
/// Max number of retries during calculating timeout
constexpr int kMaxRetryInTimeout = 5;
/// Base time for exponential backoff algorithm
constexpr long kEBBaseTime = 25;
/// Least http code that we treat as bad for exponential backoff algorithm
constexpr long kLeastBadHttpCodeForEB = 500;

constexpr Status kTestsuiteCode{599};

const std::string kTracingClientName = "external";

const std::map<std::string, std::error_code> kTestsuiteActions = {
    {"timeout", {curl::errc::EasyErrorCode::kOperationTimedout}},
    {"network", {curl::errc::EasyErrorCode::kCouldNotConnect}}};
const std::string kTestsuiteSupportedErrorsKey = "X-Testsuite-Supported-Errors";
const std::string kTestsuiteSupportedErrors =
    boost::algorithm::join(boost::adaptors::keys(kTestsuiteActions), ",");

[[noreturn]] void AbortWithStacktrace() {
  auto trace = boost::stacktrace::stacktrace();
  std::string trace_msg = boost::stacktrace::to_string(trace);
  std::cerr << "Stacktrace: " << trace_msg << "\n";
  std::abort();
}

std::error_code TestsuiteResponseHook(Status status_code,
                                      const Headers& headers,
                                      tracing::Span& span) {
  if (status_code == kTestsuiteCode) {
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

char* rfind_not_space(char* ptr, size_t size) {
  for (char* p = ptr + size - 1; p >= ptr; --p) {
    char c = *p;
    if (c == '\n' || c == '\r' || c == ' ' || c == '\t') continue;
    return p + 1;
  }
  return ptr;
}

}  // namespace

void RequestState::SetDestinationMetricNameAuto(std::string destination) {
  destination_metric_name_ = std::move(destination);
}

RequestState::RequestState(
    std::shared_ptr<impl::EasyWrapper>&& wrapper,
    std::shared_ptr<RequestStats>&& req_stats,
    const std::shared_ptr<DestinationStatistics>& dest_stats)
    : easy_(std::move(wrapper)),
      stats_(std::move(req_stats)),
      dest_stats_(dest_stats),
      timeout_ms_(
          std::chrono::duration_cast<std::chrono::milliseconds>(kDefaultTimeout)
              .count()),
      disable_reply_decoding_(false),
      is_cancelled_(false) {
  // Libcurl calls sigaction(2)  way too frequently unless this option is used.
  easy().set_no_signal(true);
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

void RequestState::ca_file(const std::string& dir_path) {
  easy().set_ca_file(dir_path.c_str());
}

void RequestState::crl_file(const std::string& file_path) {
  easy().set_crl_file(file_path.c_str());
}

void RequestState::client_key_cert(crypto::PrivateKey pkey,
                                   crypto::Certificate cert) {
  YTX_INVARIANT(pkey, "No private key");
  YTX_INVARIANT(cert, "No certificate");

  pkey_ = std::move(pkey);
  cert_ = std::move(cert);
  easy().set_ssl_ctx_function(&RequestState::on_certificate_request);
  easy().set_ssl_ctx_data(this);
}

void RequestState::http_version(curl::easy::http_version_t version) {
  LOG_DEBUG() << "http_version";
  easy().set_http_version(version);
  LOG_DEBUG() << "http_version after";
}

void RequestState::set_timeout(long timeout_ms) {
  UASSERT_MSG(timeout_ms >= 0, "timeout_ms < 0 (" + std::to_string(timeout_ms) +
                                   "), uninitialized variable?");
  timeout_ms_ = timeout_ms;
  easy().set_timeout_ms(timeout_ms);
  easy().set_connect_timeout_ms(timeout_ms);
}

void RequestState::retry(int retries, bool on_fails) {
  retry_.retries = retries;
  retry_.current = 1;
  retry_.on_fails = on_fails;
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

void RequestState::DisableReplyDecoding() { disable_reply_decoding_ = true; }

size_t RequestState::on_header(void* ptr, size_t size, size_t nmemb,
                               void* userdata) {
  auto* self = static_cast<RequestState*>(userdata);
  size_t data_size = size * nmemb;
  if (self) self->parse_header(static_cast<char*>(ptr), data_size);
  return data_size;
}

curl::native::CURLcode RequestState::on_certificate_request(
    void* /*curl*/, void* sslctx, void* userdata) noexcept {
  const auto ssl = static_cast<SSL_CTX*>(sslctx);
  auto* self = static_cast<RequestState*>(userdata);

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

void RequestState::on_completed(
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    std::shared_ptr<RequestState> holder, std::error_code err) {
  auto& span = *holder->span_;
  auto& easy = holder->easy();
  LOG_DEBUG() << "Request::RequestImpl::on_completed(1)" << span;
  const auto status_code = static_cast<Status>(easy.get_response_code());

  if (holder->testsuite_config_ && !err) {
    const auto& headers = holder->response()->headers();
    err = TestsuiteResponseHook(status_code, headers, span);
  }

  holder->AccountResponse(err);
  if (holder->dest_req_stats_) {
    const auto sockets = easy.get_num_connects();
    holder->dest_req_stats_->AccountOpenSockets(sockets);
  }

  span.AddTag(tracing::kAttempts, holder->retry_.current);
  span.AddTag(tracing::kMaxAttempts, holder->retry_.retries);
  span.AddTag(tracing::kTimeoutMs, holder->timeout_ms_);

  LOG_DEBUG() << "Request::RequestImpl::on_completed(2)" << span;
  if (err) {
    span.AddTag(tracing::kErrorFlag, true);
    span.AddTag(tracing::kErrorMessage, err.message());
    span.AddTag(tracing::kHttpStatusCode, kTestsuiteCode);  // TODO

    holder->promise_.set_exception(PrepareException(
        err, easy.get_effective_url(), easy.get_local_stats()));
  } else {
    span.AddTag(tracing::kHttpStatusCode, status_code);
    holder->response()->SetStatusCode(status_code);
    holder->response()->SetStats(easy.get_local_stats());

    if (!holder->response()->IsOk()) span.AddTag(tracing::kErrorFlag, true);

    holder->promise_.set_value(holder->response_move());
  }

  LOG_DEBUG() << "Request::RequestImpl::on_completed(3)" << span;
  holder->span_.reset();
}

void RequestState::AccountResponse(std::error_code err) {
  const auto attempts = retry_.current;

  const auto time_to_start =
      std::chrono::duration_cast<std::chrono::microseconds>(
          easy().time_to_start());

  stats_->StoreTimeToStart(time_to_start);
  if (err)
    stats_->FinishEc(err, attempts);
  else
    stats_->FinishOk(static_cast<int>(easy().get_response_code()), attempts);

  if (dest_req_stats_) {
    dest_req_stats_->StoreTimeToStart(time_to_start);
    if (err)
      dest_req_stats_->FinishEc(err, attempts);
    else
      dest_req_stats_->FinishOk(static_cast<int>(easy().get_response_code()),
                                attempts);
  }
}

void RequestState::on_retry(
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    std::shared_ptr<RequestState> holder, std::error_code err) {
  LOG_DEBUG() << "RequestImpl::on_retry" << *holder->span_;

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

    // calculate timeout before retry
    long timeout_ms =
        // NOLINTNEXTLINE(cert-msc50-cpp): we don't require good randomness here
        kEBBaseTime * (rand() % ((1 << std::min(holder->retry_.current - 1,
                                                kMaxRetryInTimeout))) +
                       1);
    // increase try
    ++holder->retry_.current;
    holder->easy().mark_retry();
    // initialize timer
    holder->retry_.timer = std::make_unique<engine::ev::TimerWatcher>(
        holder->easy().GetThreadControl());
    // call on_retry_timer on timer
    holder->retry_.timer->SingleshotAsync(
        std::chrono::milliseconds(timeout_ms),
        [holder = std::move(holder)](std::error_code err) {
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

engine::impl::BlockingFuture<std::shared_ptr<Response>>
RequestState::async_perform() {
  span_.emplace(kTracingClientName);
  easy().add_header(::http::headers::kXYaSpanId, span_->GetSpanId());
  easy().add_header(::http::headers::kXYaTraceId, span_->GetTraceId());
  easy().add_header(::http::headers::kXYaRequestId, span_->GetLink());
  // effective url is not available yet
  span_->AddTag(tracing::kHttpUrl, easy().get_original_url());

  ApplyTestsuiteConfig();

  // Span is local to a Request, it is not related to current coroutine
  span_->DetachFromCoroStack();

  // define header function
  easy().set_header_function(&RequestState::on_header);
  easy().set_header_data(this);

  if (disable_reply_decoding_) {
    // don't autodecode replies
    easy().set_accept_encoding(nullptr);
  } else {
    // set autodecoding for gzip and deflate
    easy().set_accept_encoding("gzip,deflate");
  }

  if (!dest_req_stats_) {
    dest_req_stats_ =
        dest_stats_->GetStatisticsForDestinationAuto(destination_metric_name_);
  }

  stats_->Start();
  if (dest_req_stats_) dest_req_stats_->Start();

  // if we need retries call with special callback
  if (retry_.retries <= 1)
    perform_request([holder = shared_from_this()](std::error_code err) mutable {
      RequestState::on_completed(std::move(holder), err);
    });
  else
    perform_request([holder = shared_from_this()](std::error_code err) mutable {
      RequestState::on_retry(std::move(holder), err);
    });

  return promise_.get_future();
}

void RequestState::perform_request(curl::easy::handler_type handler) {
  UASSERT_MSG(!cert_ || pkey_,
              "Setting certificate is useless without setting private key");

  response_ = std::make_shared<Response>();
  // set place for response body
  easy().set_sink(&(response_->sink_stream()));

  // perform request
  easy().async_perform(std::move(handler));
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
      std::cerr << url << " forbidden by testsuite config, allowed prefixes=\n"
                << boost::algorithm::join(prefixes, "\n") << "\n";
      AbortWithStacktrace();
    }
  }

  const auto& timeout = testsuite_config_->http_request_timeout;
  if (timeout) {
    set_timeout(std::chrono::milliseconds(*timeout).count());
  }

  easy().add_header(kTestsuiteSupportedErrorsKey, kTestsuiteSupportedErrors);
}

}  // namespace clients::http
