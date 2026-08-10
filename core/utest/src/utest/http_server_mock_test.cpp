#include <userver/utest/utest.hpp>

#include <userver/clients/http/client.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/logging/log.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/http_server_mock.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
const std::string kRequestBody = "some body";
const std::string kResponseBody = "returned body";
}  // namespace

UTEST(HttpServerMock, Sample) {
    /// [Sample HttpServerMock usage]
    constexpr http::headers::PredefinedHeader kHeader{"HeaderName"};
    const utest::HttpServerMock mock([&kHeader](const utest::HttpServerMock::HttpRequest& request) {
        EXPECT_EQ(clients::http::HttpMethod::kPost, request.method);
        EXPECT_EQ("/path", request.path);
        EXPECT_EQ("value1", request.headers.at(std::string_view{"header_a"}));
        EXPECT_EQ("body", request.body);
        EXPECT_EQ((std::multimap<std::string, std::string>{{"arg1", "val1"}}), request.query);

        utest::HttpServerMock::HttpResponse response;
        response.response_status = 200;
        response.headers[kHeader] = "HeaderValue";
        response.body = "returned from mock body";
        return response;
    });

    // Making a request to the `mock`
    auto http_client_ptr = utest::CreateHttpClient();
    auto response =
        http_client_ptr->CreateRequest()
            .post(mock.GetBaseUrl() + "/path?arg1=val1", "body")
            .headers({std::make_pair("header_a", "value1")})
            .retry(1)
            .timeout(utest::kMaxTestWaitTime)
            .perform();

    // Response from the `mock`
    EXPECT_EQ(200, response->status_code());
    EXPECT_EQ("returned from mock body", response->body());
    EXPECT_EQ("HeaderValue", response->headers()[kHeader]);
    /// [Sample HttpServerMock usage]
}

UTEST(HttpServerMock, Ctr) {
    const utest::HttpServerMock mock([](const utest::HttpServerMock::HttpRequest& request) {
        EXPECT_EQ(clients::http::HttpMethod::kPost, request.method);
        EXPECT_EQ("/", request.path);
        EXPECT_EQ("value1", request.headers.at(std::string_view{"a"}));
        EXPECT_EQ("value2", request.headers.at(std::string_view{"header"}));

        EXPECT_EQ((std::multimap<std::string, std::string>{{"arg1", "val1"}, {"arg2", "val2"}}), request.query);
        EXPECT_EQ(kRequestBody, request.body);
        return utest::HttpServerMock::HttpResponse{
            287,
            clients::http::Headers{std::make_pair(std::string{"x"}, std::string{"y"})},
            kResponseBody,
        };
    });

    auto http_client_ptr = utest::CreateHttpClient();
    const clients::http::Headers headers{
        std::make_pair(std::string{"a"}, std::string{"value1"}),
        std::make_pair(std::string{"header"}, std::string{"value2"}),
    };
    for (int i = 0; i < 2; i++) {
        auto response =
            http_client_ptr->CreateRequest()
                .post(mock.GetBaseUrl() + "?arg1=val1&arg2=val2", kRequestBody)
                .headers(headers)
                .retry(1)
                .timeout(utest::kMaxTestWaitTime)
                .perform();

        EXPECT_EQ(287, response->status_code());
        EXPECT_EQ(kResponseBody, response->body());
        EXPECT_EQ(
            (clients::http::Headers{
                std::make_pair(std::string{"x"}, std::string{"y"}),
                std::make_pair(std::string{"Content-Length"}, std::string{"13"})
            }),
            response->headers()
        );
    }
}

UTEST(HttpServerMock, BodyAcrossMultipleReads) {
    const std::string first_half = "first-part-of-body;";
    const std::string second_half = "second-part-of-body";
    const std::string full_body = first_half + second_half;

    const utest::HttpServerMock mock([&](const utest::HttpServerMock::HttpRequest& request) {
        EXPECT_EQ(request.body, full_body);
        return utest::HttpServerMock::HttpResponse{200, {}, ""};
    });

    const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

    auto addr = engine::io::Sockaddr::MakeIPv4LoopbackAddress();
    addr.SetPort(mock.GetPort());

    engine::io::Socket sock{addr.Domain(), engine::io::SocketType::kStream};
    sock.Connect(addr, deadline);

    const std::string head =
        "POST /test HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Content-Length: " +
        std::to_string(full_body.size()) +
        "\r\n"
        "\r\n";

    // Write #1: headers + first half of body.
    const std::string write1 = head + first_half;
    [[maybe_unused]] const auto sent1 = sock.SendAll(write1.data(), write1.size(), deadline);

    // Small deliberate delay so the mock's ReadSome() returns with a partial
    // body, triggering a second ReadSome() call for the remaining bytes.
    engine::SleepFor(std::chrono::milliseconds{50});

    // Write #2: second half of body.
    [[maybe_unused]] const auto sent2 = sock.SendAll(second_half.data(), second_half.size(), deadline);

    // Read the response to let the mock's handler EXPECT assertions execute.
    std::array<char, 1024> buf{};
    [[maybe_unused]] const auto received = sock.RecvSome(buf.data(), buf.size(), deadline);
}

USERVER_NAMESPACE_END
