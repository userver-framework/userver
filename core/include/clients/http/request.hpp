#pragma once

/// @file clients/http/request.hpp
/// @brief @copybrief clients::http::Request

#include <memory>

#include <clients/http/enforce_task_deadline_config.hpp>
#include <clients/http/error.hpp>
#include <clients/http/response.hpp>
#include <clients/http/response_future.hpp>
#include <crypto/certificate.hpp>
#include <crypto/private_key.hpp>

/// HTTP client helpers
namespace clients::http {

class RequestState;
namespace impl {
class EasyWrapper;
}  // namespace impl

/// HTTP request method
enum class HttpMethod { kGet, kPost, kHead, kPut, kDelete, kPatch, kOptions };

/// HTTP version to use
enum class HttpVersion {
  kDefault,  ///< unspecified version
  k10,       ///< HTTP/1.0 only
  k11,       ///< HTTP/1.1 only
  k2,        ///< HTTP/2 with fallback to HTTP/1.1
  k2Tls,     ///< HTTP/2 over TLS only, otherwise (no TLS or h2) HTTP/1.1
  k2PriorKnowledge,  ///< HTTP/2 only (without Upgrade)
};

class Form;
class RequestStats;
class DestinationStatistics;
struct TestsuiteConfig;

/// Class for creating and performing new http requests
class Request final : public std::enable_shared_from_this<Request> {
 public:
  /// Request cookies container type
  using Cookies = std::unordered_map<std::string, std::string>;

  explicit Request(std::shared_ptr<impl::EasyWrapper>&&,
                   std::shared_ptr<RequestStats>&& req_stats,
                   const std::shared_ptr<DestinationStatistics>& dest_stats);

  /// Specifies method
  std::shared_ptr<Request> method(HttpMethod method);
  /// GET request
  std::shared_ptr<Request> get();
  /// GET request with url
  std::shared_ptr<Request> get(const std::string& url);
  /// HEAD request
  std::shared_ptr<Request> head();
  /// HEAD request with url
  std::shared_ptr<Request> head(const std::string& url);
  /// POST request
  std::shared_ptr<Request> post();
  /// POST request with url and data
  std::shared_ptr<Request> post(const std::string& url,
                                std::string data = std::string());
  /// POST request with url and multipart/form-data
  std::shared_ptr<Request> post(const std::string& url, const Form& form);
  /// PUT request
  std::shared_ptr<Request> put();
  /// PUT request with url and data
  std::shared_ptr<Request> put(const std::string& url, std::string data);

  /// PATCH request
  std::shared_ptr<Request> patch();
  /// PATCH request with url and data
  std::shared_ptr<Request> patch(const std::string& url, std::string data);

  /// DELETE request
  std::shared_ptr<Request> delete_method();
  /// DELETE request with url
  std::shared_ptr<Request> delete_method(const std::string& url);

  /// url if you don't specify request type with url
  std::shared_ptr<Request> url(const std::string& url);
  /// data for POST request
  std::shared_ptr<Request> data(std::string data);
  /// form for POST request
  std::shared_ptr<Request> form(const Form& form);
  /// Headers for request as map
  std::shared_ptr<Request> headers(const Headers& headers);
  /// Headers for request as list
  std::shared_ptr<Request> headers(
      std::initializer_list<std::pair<std::string_view, std::string_view>>
          headers);
  /// Sets the User-Agent header
  std::shared_ptr<Request> user_agent(const std::string& value);
  /// Sets proxy to use. Example: [::1]:1080
  std::shared_ptr<Request> proxy(const std::string& value);
  /// Cookies for request as map
  std::shared_ptr<Request> cookies(const Cookies& cookies);
  /// Follow redirects or not. Default: follow
  std::shared_ptr<Request> follow_redirects(bool follow = true);
  /// Set timeout in ms for request
  std::shared_ptr<Request> timeout(long timeout_ms);
  std::shared_ptr<Request> timeout(std::chrono::milliseconds timeout_ms) {
    return timeout(timeout_ms.count());
  }
  /// Verify host and peer or not. Default: verify
  std::shared_ptr<Request> verify(bool verify = true);
  /// Set file holding one or more certificates to verify the peer with
  std::shared_ptr<Request> ca_info(const std::string& file_path);
  /// Set dir with CA certificates
  std::shared_ptr<Request> ca_file(const std::string& dir_path);
  /// Set CRL-file
  std::shared_ptr<Request> crl_file(const std::string& file_path);
  /// Set private client key and certificate for request
  std::shared_ptr<Request> client_key_cert(crypto::PrivateKey pkey,
                                           crypto::Certificate cert);
  /// Set HTTP version
  std::shared_ptr<Request> http_version(HttpVersion version);

  /// Specify number of retries on incorrect status, if on_failes is True
  /// retry on network error too. Retries = 3 means that maximum 3 request
  /// will be performed
  std::shared_ptr<Request> retry(short retries = 3, bool on_fails = true);

  /// Set unix domain socket as connection endpoint and provide path to it
  /// When enabled, request will connect to the Unix domain socket instead
  /// of establishing a TCP connection to a host.
  std::shared_ptr<Request> unix_socket_path(const std::string& path);

  /// Set destination name in metric "httpclient.destinations.<name>".
  /// If not set, defaults to HTTP path.  Should be called for all requests
  /// with parameters in HTTP path.
  std::shared_ptr<Request> SetDestinationMetricName(
      const std::string& destination);

  /// Set testuite related settings.
  std::shared_ptr<Request> SetTestsuiteConfig(
      const std::shared_ptr<const TestsuiteConfig>& config);

  /// Disable autodecoding of received replies.
  /// Useful to proxy replies 'as is'.
  std::shared_ptr<Request> DisableReplyDecoding();

  /// Enable auto add header with client timeout.
  std::shared_ptr<Request> EnableAddClientTimeoutHeader();

  /// Disable auto add header with client timeout.
  std::shared_ptr<Request> DisableAddClientTimeoutHeader();

  /// How to use deadline from current task with request timeout
  std::shared_ptr<Request> SetEnforceTaskDeadline(
      EnforceTaskDeadlineConfig enforce_task_deadline);

  /// Perform request asynchronously
  [[nodiscard]] ResponseFuture async_perform();

  /// Calls async_perform and wait for timeout_ms on a future. Default time
  /// for waiting will be timeout value if it was setted. If error occured it
  /// will be thrown as exception.
  [[nodiscard]] std::shared_ptr<Response> perform();

  /// Get Response class
  std::shared_ptr<Response> response() const;

 private:
  std::shared_ptr<RequestState> pimpl_;
};

}  // namespace clients::http
