#pragma once

/// @file userver/utest/simple_server.hpp
/// @brief @copybrief utest::SimpleServer

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace utest {

/// @ingroup userver_utest
///
/// @brief Toy server for simple network testing; for testing HTTP prefer using utest::HttpServerMock.
///
/// In constructor opens specified ports in localhost address and listens on
/// them. On each accepted data packet calls user callback.
///
/// ## Example usage:
/// @snippet core/src/testing_test.cpp  Sample SimpleServer usage
class SimpleServer final {
public:
    /// Response to return from the OnRequest callback
    struct Response {
        enum Commands {
            kWriteAndClose,
            kTryReadMore,
            kWriteAndContinue,
        };

        std::string data_to_send{};

        /// What the SimpleServer has to do with the connection after the `data_to_send` is sent
        Commands command{kWriteAndClose};
    };

    /// Request that is passed to the OnRequest callback
    using Request = std::string;

    /// Callback that is invoked on each network request
    using OnRequest = std::function<Response(const Request&)>;

    using Port = unsigned short;
    enum Protocol { kTcpIpV4, kTcpIpV6 };

    SimpleServer(OnRequest callback, Protocol protocol = kTcpIpV4);
    ~SimpleServer();

    /// Returns Port the server listens on.
    Port GetPort() const;

    enum class Schema {
        kHttp,
        kHttps,
    };

    /// Returns URL to the server, for example 'http://127.0.0.1:8080'
    std::string GetBaseUrl(Schema type = Schema::kHttp) const;

    std::uint64_t GetConnectionsOpenedCount() const;

private:
    class Impl;
    const std::unique_ptr<Impl> pimpl_;
};

}  // namespace utest

USERVER_NAMESPACE_END
