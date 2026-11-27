#include <storages/mongo/cdriver/async_stream.hpp>

#include <string>
#include <string_view>

#include <fmt/format.h>
#include <gmock/gmock.h>
#include <mongoc/mongoc.h>

#include <userver/storages/mongo/mongo_error.hpp>
#include <userver/utest/simple_server.hpp>
#include <userver/utest/utest.hpp>

#include <storages/mongo/cdriver/wrappers.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {
namespace {

constexpr std::string_view kMissingCaFile = "/nonexistent/userver-mongo-test-ca.pem";
constexpr int kConnectTimeoutMs = 1000;

UriPtr MakeTlsUri(utest::SimpleServer::Port port) {
    const auto uri_string = fmt::format(
        "mongodb://127.0.0.1:{}/db?tls=true&{}={}&{}={}",
        port,
        MONGOC_URI_TLSCAFILE,
        kMissingCaFile,
        MONGOC_URI_CONNECTTIMEOUTMS,
        kConnectTimeoutMs
    );
    return UriPtr{mongoc_uri_new(uri_string.c_str())};
}

}  // namespace

UTEST(MakeAsyncStream, UsesTlsOptionsOfPassedUri) {
    const utest::SimpleServer server{[](const utest::SimpleServer::Request&) { return utest::SimpleServer::Response{}; }
    };
    const auto uri = MakeTlsUri(server.GetPort());
    ASSERT_TRUE(uri);

    AsyncStreamInitiatorData init_data{nullptr, {}};
    MongoError error;
    const StreamPtr stream{MakeAsyncStream(uri.get(), mongoc_uri_get_hosts(uri.get()), &init_data, error.GetNative())};

    EXPECT_FALSE(stream);
    EXPECT_THAT(error.Message(), testing::HasSubstr("Cannot initialize TLS stream"));
}

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END
