#pragma once

#include <userver/components/tcp_acceptor_base.hpp>
#include <userver/crypto/certificate.hpp>
#include <userver/crypto/private_key.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/http/http_status.hpp>

#include <atomic>
#include <exception>
#include <functional>
#include <unordered_map>

namespace userver::websocket {

struct string_hash {
  using hash_type = std::hash<std::string_view>;
  using is_transparent = void;
  size_t operator()(const char* str) const { return hash_type{}(str); }
  size_t operator()(std::string_view str) const { return hash_type{}(str); }
  size_t operator()(std::string const& str) const { return hash_type{}(str); }
};

template <typename T>
using string_map =
    std::unordered_map<std::string, T, string_hash, std::equal_to<>>;

using userver::server::http::HttpMethod;
using userver::server::http::HttpStatus;

using Headers = string_map<std::string>;

inline bool TestHeaderVal(const Headers& headers, std::string_view key,
                          std::string_view val) {
  auto it = headers.find(std::string{key});
  if (it == headers.end()) return false;
  return it->second == val;
}

inline const std::string& GetOptKey(const Headers& headers,
                                    std::string_view key,
                                    const std::string& def) {
  auto it = headers.find(std::string{key});
  if (it != headers.end()) return it->second;
  return def;
}

struct Request {
  std::string url;
  HttpMethod method;
  Headers headers;
  std::vector<std::byte> content;

  const userver::engine::io::Sockaddr* client_address;
  bool keepalive = false;
};

struct Response {
  HttpStatus status = HttpStatus::kOk;
  Headers headers;
  std::vector<std::byte> content;
  std::string_view content_type;
  bool keepalive = false;
  std::function<void()> post_send_cb;
  std::function<void(std::unique_ptr<userver::engine::io::RwBase>)>
      upgrade_connection;
};

struct HttpStatusException : public std::exception {
  HttpStatus status;
  HttpStatusException(HttpStatus s) : status(s) {}
};

class HttpServer : public components::TcpAcceptorBase {
 public:
  struct Config {
    bool allow_encoding = true;
    struct TlsConfig {
      userver::crypto::Certificate cert;
      userver::crypto::PrivateKey key;
    };

    std::optional<TlsConfig> tls;
  };

  HttpServer(const components::ComponentConfig& component_config,
             const components::ComponentContext& component_context);

  enum class OperationMode {
    Normal,
    Throttled  // Server will response with status kTooManyRequests on all
               // requests and close connection
  };

  void SetOperationMode(OperationMode opmode);

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  virtual Response HandleRequest(const Request& request) = 0;

  void ProcessSocket(engine::io::Socket&& sock) final;

  Config config;
  std::atomic<OperationMode> operation_mode = OperationMode::Normal;
};

}  // namespace userver::websocket
