#pragma once

/// @file userver/utest/simple_server.hpp
/// @brief @copybrief utest::SimpleServer

#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace utest {

/// @ingroup userver_utest
///
/// Toy server for simple network testing.
///
/// In constructor opens specified ports in localhost address and listens on
/// them. On each accepted data packet calls user callback.
///
/// ## Example usage:
/// @snippet testing_test.cpp  Sample SimpleServer usage
class SimpleServer final {
 public:
  struct Response {
    enum Commands {
      kWriteAndClose,
      kTryReadMore,
      kWriteAndContinue,
    };

    std::string data_to_send{};
    Commands command{kWriteAndClose};
  };

  using Request = std::string;
  using OnRequest = std::function<Response(const Request&)>;

  using Port = unsigned short;
  enum Protocol { kTcpIpV4, kTcpIpV6 };

  SimpleServer(OnRequest callback, Protocol protocol = kTcpIpV4);
  ~SimpleServer();

  Port GetPort() const;

  enum class Schema {
    kHttp,
    kHttps,
  };

  std::string GetBaseUrl(Schema type = Schema::kHttp) const;

 private:
  class Impl;
  const std::unique_ptr<Impl> pimpl_;
};

}  // namespace utest

USERVER_NAMESPACE_END
