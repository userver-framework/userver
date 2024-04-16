#include <clients/http/request_state.hpp>

#include <algorithm>
#include <chrono>
#include <map>
#include <string_view>

#include <cryptopp/osrng.h>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <curl-ev/error_code.hpp>
#include <userver/baggage/baggage.hpp>
#include <userver/clients/dns/resolver.hpp>
#include <userver/clients/http/connect_to.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/encoding/hex.hpp>
#include <userver/utils/overloaded.hpp>
#include <userver/utils/rand.hpp>
#include <utils/impl/assert_extra.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

namespace {
namespace nolint {
// NOLINT(bugprone-forward-declaration-namespace) due to CryptoPP::Source
using UnusedConfigSourceFwd = dynamic_config::Source&;
}  // namespace nolint

/// Default timeout
constexpr auto kDefaultTimeout = std::chrono::milliseconds{100};
/// Maximum number of redirects
constexpr long kMaxRedirectCount = 10;
/// Max power value for exponential backoff algorithm
constexpr int kEBMaxPower = 5;
/// Base time for exponential backoff algorithm
constexpr auto kEBBaseTime = std::chrono::milliseconds{25};
/// Least http code that we treat as bad for exponential backoff algorithm
constexpr Status kLeastBadHttpCodeForEB{500};
/// Least http code the the downstream service can use to report propagated
/// deadline expiration
constexpr Status kLeastHttpCodeForDeadlineExpired{400};

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
    const auto it = headers.find(std::string_view{"X-Testsuite-Error"});

    if (headers.end() != it) {
      LOG_INFO() << "Mockserver faked error of type " << it->second
                 << tracing::impl::LogSpanAsLastNonCoro{span};

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

bool IsSetCookie(std::string_view key) {
  const utils::StrIcaseEqual equal;
  return equal(key, USERVER_NAMESPACE::http::headers::kSetCookie);
}

// Not a strict check, but OK for non-header line check
bool IsHttpStatusLineStart(const char* ptr, size_t size) {
  return (size > 5 && memcmp(ptr, "HTTP/", 5) == 0);
}

char* rfind_not_space(char* ptr, size_t size) {
  for (char* p = ptr + size - 1; p >= ptr; --p) {
    const char c = *p;
    if (c == '\n' || c == '\r' || c == ' ' || c == '\t') continue;
    return p + 1;
  }
  return ptr;
}

// TODO: very low-level, do it in another place
void SetBaggageHeader(curl::easy& e) {
  const auto* baggage = baggage::kInheritedBaggage.GetOptional();
  if (baggage != nullptr) {
    LOG_DEBUG() << fmt::format("Send baggage: {}", baggage->ToString());
    e.add_header(USERVER_NAMESPACE::http::headers::kXBaggage,
                 baggage->ToString(), curl::easy::EmptyHeaderAction::kDoNotSend,
                 curl::easy::DuplicateHeaderAction::kReplace);
  }
}

std::exception_ptr PrepareDeadlinePassedException(std::string_view url,
                                                  LocalStats stats) {
  return std::make_exception_ptr(CancelException(
      fmt::format("Timeout happened (deadline propagation), url: {}", url),
      stats));
}

bool IsPrefix(const std::string& url,
              const std::vector<std::string>& prefixes) {
  return !(std::find_if(prefixes.begin(), prefixes.end(),
                        [&url](const std::string& prefix) {
                          return boost::starts_with(url, prefix);
                        }) == prefixes.end());
}

class MaybeOwnedUrl final {
 public:
  MaybeOwnedUrl(const std::string& proxy_url, curl::easy& easy) {
    if (!proxy_url.empty()) {
      url_if_with_proxy_.emplace();

      std::error_code ec;
      url_if_with_proxy_->SetDefaultSchemeUrl(proxy_url.c_str(), ec);
      if (ec) {
        throw BadArgumentException(ec, "Bad proxy URL", proxy_url, {});
      }

      url_ptr_ = &*url_if_with_proxy_;
    } else {
      url_ptr_ = &easy.get_easy_url();
    }
  }
  MaybeOwnedUrl(const MaybeOwnedUrl&) = delete;
  MaybeOwnedUrl(MaybeOwnedUrl&&) = delete;

  const curl::url& Get() const {
    UASSERT(url_ptr_);
    return *url_ptr_;
  }

 private:
  std::optional<curl::url> url_if_with_proxy_;
  const curl::url* url_ptr_{nullptr};
};

// Instance-wide password used to avoid passing unencrypted keys
[[maybe_unused]] const std::string& GetPkeyPassword() {
  static const std::string password = [] {
    CryptoPP::DefaultAutoSeededRNG prng;
    std::string random_bytes;
    random_bytes.resize(32);
    // password should not contain '\0' (cURL only accepts C-style strings)
    while (random_bytes.find('\0') != std::string::npos) {
      static_assert(sizeof(std::string::value_type) == 1,
                    "string does not consist of bytes");
      prng.GenerateBlock(reinterpret_cast<unsigned char*>(random_bytes.data()),
                         random_bytes.size());
    }
    return random_bytes;
  }();
  return password;
}

// Type-dependent implementations to avoid alternate branch instantiation
// (tries to use deleted methods otherwise).
template <typename Easy>
void ModernCaImpl(Easy& easy, crypto::Certificate&& cert) {
  static_assert(Easy::is_set_ca_info_blob_available,
                "Modern implementation called in legacy env");
  auto cert_pem = cert.GetPemString();
  UINVARIANT(cert_pem, "Could not serialize certificate");
  easy.set_ca_info_blob_copy(*cert_pem);
}

template <typename Easy>
void ModernClientKeyCertImpl(Easy& easy, crypto::PrivateKey&& pkey,
                             crypto::Certificate&& cert) {
  static_assert(Easy::is_set_ssl_cert_blob_available &&
                    Easy::is_set_ssl_key_blob_available,
                "Modern implementation called in legacy env");
  auto cert_pem = cert.GetPemString();
  UINVARIANT(cert_pem, "Could not serialize certificate");
  easy.set_ssl_cert_blob_copy(*cert_pem);
  easy.set_ssl_cert_type("PEM");
  auto key_pem = pkey.GetPemString(GetPkeyPassword());
  UINVARIANT(key_pem, "Could not serialize private key");
  easy.set_ssl_key_blob_copy(*key_pem);
  easy.set_ssl_key_passwd(GetPkeyPassword());
  easy.set_ssl_key_type("PEM");
}

}  // namespace

RequestState::RequestState(
    impl::EasyWrapper&& wrapper, RequestStats&& req_stats,
    const std::shared_ptr<DestinationStatistics>& dest_stats,
    clients::dns::Resolver* resolver, impl::PluginPipeline& plugin_pipeline,
    const tracing::TracingManagerBase& tracing_manager)
    : easy_(std::move(wrapper)),
      stats_(std::move(req_stats)),
      dest_stats_(dest_stats),
      original_timeout_(kDefaultTimeout),
      remote_timeout_(original_timeout_),
      tracing_manager_{tracing_manager},
      is_cancelled_(false),
      errorbuffer_(),
      resolver_{resolver},
      plugin_pipeline_{plugin_pipeline} {
  // Libcurl calls sigaction(2)  way too frequently unless this option is used.
  easy().set_no_signal(true);
  easy().set_error_buffer(errorbuffer_.data());

  // define header function
  easy().set_header_function(&RequestState::on_header);
  easy().set_header_data(this);

  // set autodecoding for gzip and deflate
  easy().set_accept_encoding("gzip,deflate,identity");
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
  UINVARIANT(cert, "No certificate");
  if constexpr (curl::easy::is_set_ca_info_blob_available) {
    ModernCaImpl(easy(), std::move(cert));
  } else {
    // Legacy non-portable way, broken since 7.87.0
    ca_ = std::move(cert);
    easy().set_ssl_ctx_function(&RequestState::on_certificate_request);
    easy().set_ssl_ctx_data(this);
  }
}

void RequestState::crl_file(const std::string& file_path) {
  easy().set_crl_file(file_path.c_str());
}

void RequestState::client_key_cert(crypto::PrivateKey pkey,
                                   crypto::Certificate cert) {
  UINVARIANT(pkey, "No private key");
  UINVARIANT(cert, "No certificate");

  if constexpr (curl::easy::is_set_ssl_cert_blob_available &&
                curl::easy::is_set_ssl_key_blob_available) {
    ModernClientKeyCertImpl(easy(), std::move(pkey), std::move(cert));
  } else {
    // Legacy non-portable way, broken since 7.84.0
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
}

void RequestState::http_version(curl::easy::http_version_t version) {
  easy().set_http_version(version);
}

void RequestState::set_timeout(long timeout_ms) {
  original_timeout_ = std::chrono::milliseconds{timeout_ms};
  remote_timeout_ = original_timeout_;
}

void RequestState::retry(short retries, bool on_fails) {
  retry_.retries = retries;
  retry_.current = 1;
  retry_.on_fails = on_fails;
}

void RequestState::unix_socket_path(const std::string& path) {
  easy().set_unix_socket_path(path);
}

void RequestState::connect_to(const ConnectTo& connect_to) {
  curl::native::curl_slist* ptr = connect_to.GetUnderlying();
  if (ptr) {
    easy().set_connect_to(ptr);
  }
}

void RequestState::proxy(const std::string& value) {
  proxy_url_ = value;
  easy().set_proxy(value);
}

void RequestState::proxy_auth_type(curl::easy::proxyauth_t value) {
  easy().set_proxy_auth(value);
}

void RequestState::http_auth_type(curl::easy::httpauth_t value, bool auth_only,
                                  std::string_view user,
                                  std::string_view password) {
  easy().set_http_auth(value, auth_only);
  easy().set_user(std::string{user}.c_str());
  easy().set_password(std::string{password}.c_str());
}

void RequestState::Cancel() {
  // We can not call `retry_.timer.reset();` here because of data race
  is_cancelled_ = true;
  easy().cancel();
}

void RequestState::SetDestinationMetricNameAuto(std::string destination) {
  destination_metric_name_ = std::move(destination);
}

void RequestState::SetDestinationMetricName(const std::string& destination) {
  dest_req_stats_ = dest_stats_->GetStatisticsForDestination(destination);
}

void RequestState::SetTestsuiteConfig(
    const std::shared_ptr<const TestsuiteConfig>& config) {
  testsuite_config_ = config;
}

void RequestState::SetAllowedUrlsExtra(const std::vector<std::string>& urls) {
  allowed_urls_extra_ = urls;
}

void RequestState::DisableReplyDecoding() {
  easy().set_accept_encoding(nullptr);
}

void RequestState::SetCancellationPolicy(CancellationPolicy cp) {
  cancellation_policy_ = cp;
}

CancellationPolicy RequestState::GetCancellationPolicy() const {
  return cancellation_policy_;
}

void RequestState::SetDeadlinePropagationConfig(
    const DeadlinePropagationConfig& deadline_propagation_config) {
  deadline_propagation_config_ = deadline_propagation_config;
}

size_t RequestState::on_header(void* ptr, size_t size, size_t nmemb,
                               void* userdata) {
  auto* self = static_cast<RequestState*>(userdata);
  const std::size_t data_size = size * nmemb;
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

  // TODO don't swallow errors, report them to StreamedResponse
  auto* stream_data = std::get_if<StreamData>(&holder->data_);
  if (stream_data && !stream_data->headers_promise_set.exchange(true)) {
    stream_data->headers_promise.set_value();
    LOG_DEBUG() << "Stream API, status code is set (with body)";
  }

  const auto status_code = static_cast<Status>(easy.get_response_code());

  holder->CheckResponseDeadline(err, status_code);

  if (holder->testsuite_config_ && !err) {
    const auto& headers = holder->response()->headers();
    err = TestsuiteResponseHook(status_code, headers, span);
  }

  holder->AccountResponse(err);
  const auto sockets = easy.get_num_connects();
  holder->WithRequestStats(
      [sockets](RequestStats& stats) { stats.AccountOpenSockets(sockets); });

  span.AddTag(tracing::kAttempts, holder->retry_.current);
  if (holder->deadline_propagation_config_.update_header) {
    span.AddTag("propagated_timeout_ms", holder->remote_timeout_.count());
  }

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

    holder->span_storage_.reset();

    const utils::Overloaded visitor{
        [&holder, &err](FullBufferedData& buffered_data) {
          { [[maybe_unused]] const auto cleanup = holder->response_move(); }
          auto promise = std::move(buffered_data.promise_);
          // The task will wake up and may reuse RequestState.
          promise.set_exception(holder->PrepareException(err));
        },
        [](StreamData& stream_data) {
          auto producer = std::move(stream_data.queue_producer);
          // The task will wake up and may reuse RequestState.
          std::move(producer).Reset();
        }};
    std::visit(visitor, holder->data_);
  } else {
    span.AddTag(tracing::kHttpStatusCode, status_code);
    holder->response()->SetStatusCode(status_code);
    holder->response()->SetStats(easy.get_local_stats());

    if (holder->response()->IsError()) span.AddTag(tracing::kErrorFlag, true);

    holder->plugin_pipeline_.HookOnCompleted(*holder, *holder->response());

    holder->span_storage_.reset();

    const utils::Overloaded visitor{
        [&holder](FullBufferedData& buffered_data) {
          auto promise = std::move(buffered_data.promise_);
          // The task will wake up and may reuse RequestState.
          promise.set_value(holder->response_move());
        },
        [](StreamData& stream_data) {
          auto producer = std::move(stream_data.queue_producer);
          // The task will wake up and may reuse RequestState.
          std::move(producer).Reset();
        }};
    std::visit(visitor, holder->data_);
  }
  // it is unsafe to touch any content of holder after this point!
}

void RequestState::on_retry(std::shared_ptr<RequestState> holder,
                            std::error_code err) {
  UASSERT(holder);
  UASSERT(holder->span_storage_);
  LOG_TRACE() << "RequestImpl::on_retry"
              << tracing::impl::LogSpanAsLastNonCoro{
                     holder->span_storage_->Get()};

  // We do not need to retry:
  // - if we got result and HTTP code is good
  // - if we used all attempts
  // - if failed to reach server, and we should not retry on fails
  // - if this request was cancelled
  const bool not_need_retry =
      (!err && !holder->ShouldRetryResponse()) ||
      (holder->retry_.current >= holder->retry_.retries) ||
      (err && !holder->retry_.on_fails) || holder->is_cancelled_.load();

  if (not_need_retry) {
    // finish if no need to retry
    RequestState::on_completed(std::move(holder), err);
  } else {
    // calculate backoff before retry
    const auto eb_power =
        std::clamp(holder->retry_.current - 1, 0, kEBMaxPower);
    const auto backoff = kEBBaseTime * (utils::RandRange(1 << eb_power) + 1);

    holder->UpdateTimeoutFromDeadline(backoff);
    if (holder->remote_timeout_ <= std::chrono::milliseconds::zero()) {
      holder->deadline_expired_ = true;
      RequestState::on_completed(std::move(holder), err);
      return;
    }

    holder->AccountResponse(err);

    // increase try
    ++holder->retry_.current;
    holder->easy().mark_retry();

    holder->retry_.timer.emplace(holder->easy().GetThreadControl());

    // call on_retry_timer on timer
    auto& holder_ref = *holder;
    holder_ref.retry_.timer->SingleshotAsync(
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

void RequestState::ParseSingleCookie(const char* ptr, size_t size) {
  if (auto cookie =
          server::http::Cookie::FromString(std::string_view(ptr, size))) {
    [[maybe_unused]] auto [it, ok] =
        response_->cookies().emplace(cookie->Name(), std::move(*cookie));
    if (!ok) {
      LOG_WARNING() << "Failed to add cookie '" + it->first +
                           "', already added";
    }
  }
}

void RequestState::parse_header(char* ptr, size_t size) try {
  /* It is a fast path in curl's thread (io thread).  Creation of tmp
   * std::string, boost::trim_right_if(), etc. is too expensive. */

  auto* end = rfind_not_space(ptr, size);
  if (ptr == end) {
    const auto status_code = static_cast<Status>(easy().get_response_code());
    response()->SetStatusCode(status_code);
    return;
  }
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

  if (IsSetCookie(key)) {
    return ParseSingleCookie(col_pos, end - col_pos);
  }

  // From https://tools.ietf.org/html/rfc7230#page-22 :
  //
  // header-field   = field-name ":" OWS field-value OWS
  // OWS            = *( SP / HTAB )
  while (end != col_pos && (*col_pos == ' ' || *col_pos == '\t')) {
    ++col_pos;
  }

  std::string value(col_pos, end - col_pos);
  response_->headers().emplace(std::move(key), std::move(value));
} catch (const std::exception& e) {
  LOG_ERROR() << "Failed to parse header: " << e.what();
}

void RequestState::SetLoggedUrl(std::string url) { log_url_ = std::move(url); }

const std::string& RequestState::GetLoggedOriginalUrl() const noexcept {
  // We may want to use original_url if effective_url is not available yet.
  return log_url_ ? *log_url_ : easy().get_original_url();
}

engine::Future<std::shared_ptr<Response>> RequestState::async_perform(
    utils::impl::SourceLocation location) {
  data_.emplace<FullBufferedData>();

  StartNewSpan(location);
  ResetDataForNewRequest();

  auto& span = span_storage_->Get();
  span.AddTag("stream_api", 0);

  // set place for response body
  easy().set_sink(&response_->sink_string());

  auto future = std::get_if<FullBufferedData>(&data_)->promise_.get_future();

  if (UpdateTimeoutFromDeadlineAndCheck()) {
    perform_request([holder = shared_from_this()](std::error_code err) mutable {
      RequestState::on_retry(std::move(holder), err);
    });
  }

  return future;
}

engine::Future<void> RequestState::async_perform_stream(
    const std::shared_ptr<Queue>& queue, utils::impl::SourceLocation location) {
  data_.emplace<StreamData>(queue->GetProducer());

  StartNewSpan(location);
  ResetDataForNewRequest();

  auto& span = span_storage_->Get();
  span.AddTag("stream_api", 1);

  easy().set_write_function(&RequestState::StreamWriteFunction);
  easy().set_write_data(this);
  // Force no retries
  retry_.retries = 1;

  auto future = std::get_if<StreamData>(&data_)->headers_promise.get_future();

  if (UpdateTimeoutFromDeadlineAndCheck()) {
    perform_request([holder = shared_from_this()](std::error_code err) mutable {
      RequestState::on_completed(std::move(holder), err);
    });
  }

  return future;
}

void RequestState::perform_request(curl::easy::handler_type handler) {
  UASSERT_MSG(!cert_ || pkey_,
              "Setting certificate is useless without setting private key");

  UASSERT(response_);
  response_->sink_string().clear();
  response_->body().clear();

  UpdateTimeoutHeader();

  plugin_pipeline_.HookPerformRequest(*this);

  if (resolver_ && retry_.current == 1) {
    engine::AsyncNoSpan([this, holder = shared_from_this(),
                         handler = std::move(handler)]() mutable {
      try {
        ResolveTargetAddress(*resolver_);
        easy().async_perform(std::move(handler));
      } catch (const clients::dns::ResolverException& ex) {
        // TODO: should retry - TAXICOMMON-4932
        auto* buffered_data = std::get_if<FullBufferedData>(&data_);
        if (buffered_data) {
          buffered_data->promise_.set_exception(std::current_exception());
        }
      } catch (const BaseException& ex) {
        auto* buffered_data = std::get_if<FullBufferedData>(&data_);
        if (buffered_data) {
          buffered_data->promise_.set_exception(std::current_exception());
        }
      }
    }).Detach();
  } else {
    easy().async_perform(std::move(handler));
  }
}

void RequestState::SetEasyTimeout(std::chrono::milliseconds timeout) {
  UASSERT_MSG(
      timeout >= std::chrono::seconds{0},
      fmt::format("timeout_ms < 0 ({})), uninitialized variable?", timeout));
  easy().set_timeout_ms(timeout.count());
  easy().set_connect_timeout_ms(timeout.count());
}

engine::Deadline RequestState::GetDeadline() const noexcept {
  return deadline_;
}

bool RequestState::IsDeadlineExpired() const noexcept {
  return deadline_expired_;
}

void RequestState::UpdateTimeoutFromDeadline(
    std::chrono::milliseconds backoff) {
  UASSERT(remote_timeout_ >= std::chrono::milliseconds::zero());
  if (!deadline_.IsReachable()) return;

  const auto timeout_from_deadline =
      std::clamp(std::chrono::duration_cast<std::chrono::milliseconds>(
                     deadline_.TimeLeft() - backoff),
                 std::chrono::milliseconds{0}, original_timeout_);

  if (timeout_from_deadline != original_timeout_) {
    remote_timeout_ = timeout_from_deadline;
    timeout_updated_by_deadline_ = true;
    WithRequestStats(
        [](RequestStats& stats) { stats.AccountTimeoutUpdatedByDeadline(); });
  }
}

bool RequestState::UpdateTimeoutFromDeadlineAndCheck(
    std::chrono::milliseconds backoff) {
  UpdateTimeoutFromDeadline(backoff);
  if (remote_timeout_ <= std::chrono::milliseconds::zero()) {
    deadline_expired_ = true;
    HandleDeadlineAlreadyPassed();
    return false;
  }
  return true;
}

void RequestState::UpdateTimeoutHeader() {
  if (!deadline_propagation_config_.update_header) return;

  easy().add_header(USERVER_NAMESPACE::http::headers::kXYaTaxiClientTimeoutMs,
                    fmt::to_string(remote_timeout_.count()),
                    curl::easy::DuplicateHeaderAction::kReplace);
}

void RequestState::HandleDeadlineAlreadyPassed() {
  auto& span = span_storage_->Get();
  span.AddTag(tracing::kAttempts, retry_.current - 1);
  span.AddTag(tracing::kErrorFlag, true);
  span.AddTag("propagated_timeout_ms", 0);
  span.AddTag("cancelled_by_deadline", 1);

  WithRequestStats(
      [](RequestStats& stats) { stats.AccountCancelledByDeadline(); });

  auto exc = PrepareDeadlinePassedException(GetLoggedOriginalUrl(),
                                            easy().get_local_stats());

  const utils::Overloaded visitor{
      [&exc](FullBufferedData& buffered_data) {
        auto promise = std::move(buffered_data.promise_);
        // The task will wake up and may reuse RequestState.
        promise.set_exception(std::move(exc));
      },
      [&exc](StreamData& stream_data) {
        if (!stream_data.headers_promise_set.exchange(true)) {
          auto promise = std::move(stream_data.headers_promise);
          // The task will wake up and may reuse RequestState.
          promise.set_exception(std::move(exc));
        }
      }};
  std::visit(visitor, data_);
}

void RequestState::CheckResponseDeadline(std::error_code& err,
                                         Status status_code) {
  const std::chrono::microseconds attempt_time{easy().get_total_time_usec()};

  if (!deadline_expired_ && timeout_updated_by_deadline_ &&
      (attempt_time >= remote_timeout_ ||
       (!err && IsDeadlineExpiredResponse(status_code)))) {
    // The most probable cause is IsDeadlineExpiredResponse, case (1).
    // Even if not, the ResponseFuture already has thrown or is preparing
    // to throw a CancelledException, so reflect it here for consistency.
    deadline_expired_ = true;
  }

  if (deadline_expired_) {
    err = std::error_code{curl::errc::EasyErrorCode::kOperationTimedout};
    span_storage_->Get().AddTag("cancelled_by_deadline", 1);
    WithRequestStats(
        [](RequestStats& stats) { stats.AccountCancelledByDeadline(); });
  } else if (!err && IsDeadlineExpiredResponse(status_code)) {
    // IsDeadlineExpiredResponse, case (2) happened.
    err = std::error_code{curl::errc::EasyErrorCode::kOperationTimedout};
  }
}

bool RequestState::IsDeadlineExpiredResponse(Status status_code) {
  // There are two cases where deadline expires in the downstream service:
  //
  // 1. We used all of our own deadline for the attempt. Our deadline and
  // the downstream service deadline have expired at about the same time.
  // This case SHOULD NOT be retried.
  //
  // 2. We set a "small" timeout for the attempt that is less than our
  // own deadline. The downstream service deadline (taken from the
  // timeout) has expired, but deadline of the current task has not yet
  // expired. This case SHOULD be retried.
  return (deadline_propagation_config_.update_header &&
          status_code >= kLeastHttpCodeForDeadlineExpired &&
          response_->headers().contains(
              USERVER_NAMESPACE::http::headers::kXYaTaxiDeadlineExpired));
}

bool RequestState::ShouldRetryResponse() {
  const auto status_code = static_cast<Status>(easy().get_response_code());

  if (IsDeadlineExpiredResponse(status_code)) {
    // See IsDeadlineExpiredResponse, case (2).
    return !timeout_updated_by_deadline_ && retry_.on_fails;
  }

  return status_code >= kLeastBadHttpCodeForEB;
}

void RequestState::AccountResponse(std::error_code err) {
  const auto attempts = retry_.current;

  const auto time_to_start =
      std::chrono::duration_cast<std::chrono::microseconds>(
          easy().time_to_start());

  WithRequestStats([this, err, attempts, time_to_start](RequestStats& stats) {
    stats.StoreTimeToStart(time_to_start);
    if (err)
      stats.FinishEc(err, attempts);
    else
      stats.FinishOk(static_cast<int>(easy().get_response_code()), attempts);
  });
}

std::exception_ptr RequestState::PrepareException(std::error_code err) {
  if (deadline_expired_) {
    return PrepareDeadlinePassedException(easy().get_effective_url(),
                                          easy().get_local_stats());
  }

  return http::PrepareException(err, easy().get_effective_url(),
                                easy().get_local_stats());
}

void RequestState::ThrowDeadlineExpiredException() {
  // This method may be called in parallel with request handling, fetching
  // effective_url_ or local stats is unsafe.
  std::rethrow_exception(
      PrepareDeadlinePassedException(GetLoggedOriginalUrl(), LocalStats{}));
}

void RequestState::ResetDataForNewRequest() {
  SetBaggageHeader(easy());

  response_ = std::make_shared<Response>();
  response_->SetStatusCode(Status::InternalServerError);

  is_cancelled_ = false;
  retry_.current = 1;
  remote_timeout_ = original_timeout_;
  deadline_ = server::request::GetTaskInheritedDeadline();
  deadline_expired_ = false;
  timeout_updated_by_deadline_ = false;

  ApplyTestsuiteConfig();

  // Testsuite config might have changed the timeout, so add the tag here.
  // Note: HookPerformRequest can potentially change timeout manually
  // per-attempt. These changes are currently ignored.
  span_storage_->Get().AddTag(tracing::kTimeoutMs, original_timeout_.count());

  // Ignore deadline propagation when setting cURL timeout to avoid closing
  // connections on deadline expiration. The connection will still be closed if
  // the original timeout is exceeded.
  SetEasyTimeout(original_timeout_);

  StartStats();
}

size_t RequestState::StreamWriteFunction(char* ptr, size_t size, size_t nmemb,
                                         void* userdata) {
  const size_t actual_size = size * nmemb;
  RequestState& rs = *static_cast<RequestState*>(userdata);
  auto* stream_data = std::get_if<StreamData>(&rs.data_);
  UASSERT(stream_data);

  LOG_DEBUG() << fmt::format(
                     "Got bytes in stream API chunk, chunk of ({} bytes)",
                     actual_size)
              << tracing::impl::LogSpanAsLastNonCoro{rs.span_storage_->Get()};

  std::string buffer(ptr, actual_size);
  auto& queue_producer = stream_data->queue_producer;

  if (!stream_data->headers_promise_set.exchange(true)) {
    stream_data->headers_promise.set_value();
    LOG_DEBUG() << "Stream API, status code is set (with body)";
  }

  if (queue_producer.PushNoblock(std::move(buffer))) {
    return actual_size;
  }
  LOG_DEBUG() << "PushNoblock() has failed";

  if (queue_producer.Queue()->NoMoreConsumers()) {
    return actual_size;
  }

  LOG_DEBUG() << "There are some alive consumers";

  UINVARIANT(false, "not implemented CURL_WRITEFUNC_PAUSE TAXICOMMON-5611");

  return CURL_WRITEFUNC_PAUSE;
}

void RequestState::ApplyTestsuiteConfig() {
  if (!testsuite_config_) {
    return;
  }

  const auto& prefixes = testsuite_config_->allowed_url_prefixes;
  if (!prefixes.empty()) {
    auto url = easy().get_original_url();
    if (!IsPrefix(url, prefixes) && !IsPrefix(url, allowed_urls_extra_)) {
      utils::impl::AbortWithStacktrace(
          fmt::format("{} forbidden by testsuite config, allowed prefixes={}, "
                      "extra prefixes={}",
                      url, fmt::join(prefixes, ", "),
                      fmt::join(allowed_urls_extra_, ", ")));
    }
  }

  const auto& timeout = testsuite_config_->http_request_timeout;
  if (timeout) {
    set_timeout(std::chrono::milliseconds(*timeout).count());
  }

  easy().add_header(kTestsuiteSupportedErrorsKey, kTestsuiteSupportedErrors);
}

void RequestState::StartNewSpan(utils::impl::SourceLocation location) {
  UINVARIANT(
      !span_storage_,
      "Attempt to reuse request while the previous one has not finished");

  span_storage_.emplace(std::string{kTracingClientName}, location);
  auto& span = span_storage_->Get();

  auto request_editable_instance = GetEditableTracingInstance();

  if (headers_propagator_) {
    headers_propagator_->PropagateHeaders(request_editable_instance);
  }
  tracing_manager_->FillRequestWithTracingContext(span,
                                                  request_editable_instance);
  plugin_pipeline_.HookCreateSpan(*this);
  span.AddTag(tracing::kHttpUrl, GetLoggedOriginalUrl());
  span.AddTag(tracing::kMaxAttempts, retry_.retries);

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
  func(stats_);
  if (dest_req_stats_) func(*dest_req_stats_);
}

void RequestState::ResolveTargetAddress(clients::dns::Resolver& resolver) {
  const auto deadline = engine::Deadline::FromDuration(remote_timeout_);

  const MaybeOwnedUrl target{proxy_url_, easy()};
  const std::string hostname = target.Get().GetHostPtr().get();

  // CURLOPT_RESOLV hostnames cannot contain colons (as IPv6 addresses do), skip
  if (hostname.find(':') != std::string::npos) return;

  const auto addrs = resolver.Resolve(hostname, deadline);
  auto addr_strings =
      addrs | boost::adaptors::transformed(
                  [](const auto& addr) { return addr.PrimaryAddressString(); });

  easy().add_resolve(hostname, target.Get().GetPortPtr().get(),
                     fmt::to_string(fmt::join(addr_strings, ",")));
}

void RequestState::SetTracingManager(const tracing::TracingManagerBase& m) {
  tracing_manager_ = m;
}

void RequestState::SetHeadersPropagator(
    const server::http::HeadersPropagator* propagator) {
  headers_propagator_ = propagator;
}

RequestTracingEditor RequestState::GetEditableTracingInstance() {
  return RequestTracingEditor(easy());
}

}  // namespace clients::http

USERVER_NAMESPACE_END
