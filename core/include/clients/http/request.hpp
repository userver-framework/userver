#pragma once

#include <memory>

#include <clients/http/error.hpp>
#include <clients/http/response.hpp>
#include <clients/http/response_future.hpp>
#include <clients/http/wrapper.hpp>
#include <crypto/certificate.hpp>
#include <crypto/private_key.hpp>

namespace curl {
class multi;
}  // namespace curl

namespace clients {
namespace http {

enum HttpMethod { DELETE, GET, HEAD, POST, PUT, PATCH, OPTIONS };

class Form;
class RequestStats;
class DestinationStatistics;
struct TestsuiteConfig;

/// Class for creating and performing new http requests
class Request final : public std::enable_shared_from_this<Request> {
 public:
  explicit Request(std::shared_ptr<EasyWrapper>,
                   std::shared_ptr<RequestStats> req_stats,
                   std::shared_ptr<DestinationStatistics> dest_stats);

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
  std::shared_ptr<Request> post(const std::string& url,
                                const std::shared_ptr<Form>& form);
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
  std::shared_ptr<Request> form(const std::shared_ptr<Form>& form);
  /// Headers for request as map
  std::shared_ptr<Request> headers(const Headers& headers);
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
  using http_version_t = curl::easy::http_version_t;
  std::shared_ptr<Request> http_version(http_version_t version);

  /// Specify number of retries on incorrect status, if on_failes is True
  /// retry on network error too. Retries = 3 means that maximum 3 request
  /// will be performed
  std::shared_ptr<Request> retry(int retries = 3, bool on_fails = true);

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

  /// Perform request async, after completing callack will be called
  /// or it can be waiting on a future.
  [[nodiscard]] ResponseFuture async_perform();

  /// Calls async_perform and wait for timeout_ms on a future. Default time
  /// for waiting will be timeout value if it was setted. If error occured it
  /// will be thrown as exception.

  [[nodiscard]] std::shared_ptr<Response> perform();

  /// Get curl handler for specific settings
  curl::easy& easy();
  const curl::easy& easy() const;
  /// Get Response class
  std::shared_ptr<Response> response() const;

  /// Cancel request
  void Cancel() const;

 private:
  class RequestImpl;

  std::shared_ptr<RequestImpl> pimpl_;
};

}  // namespace http
}  // namespace clients
