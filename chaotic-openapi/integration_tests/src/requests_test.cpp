#include <userver/utest/utest.hpp>

#include <userver/clients/http/client.hpp>
#include <userver/clients/http/client_core.hpp>
#include <userver/dump/operations_mock.hpp>
#include <userver/http/content_type.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/http_server_mock.hpp>
#include <userver/utest/log_capture_fixture.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/testing.hpp>
#include <userver/utils/text_light.hpp>

#include <clients/multiple_content_types/requests.hpp>
#include <clients/operation/client_impl.hpp>
#include <clients/parameters/requests.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace client = ::clients::multiple_content_types::test1::post;

UTEST(Requests, RegexDestinationName) {
    const utest::HttpServerMock http_server([&](const utest::HttpServerMock::HttpRequest&) {
        return utest::HttpServerMock::HttpResponse{200};
    });

    chaotic::openapi::client::Config config;
    config.base_url = http_server.GetBaseUrl() + "/";

    auto http_client_ptr = utest::impl::CreateHttpClientCore();
    ::clients::operation::ClientImpl client(config, *http_client_ptr);

    client.WithRegex({"123"});

    utils::statistics::Storage stat_storage;
    auto stat_holder = stat_storage.RegisterWriter(
        "test",
        [&http_client_ptr](utils::statistics::Writer& writer) {
            DumpMetric(writer, http_client_ptr->GetDestinationStatistics());
        },
        {}
    );
    utils::statistics::Snapshot stats(stat_storage);
    auto expected_destination_name = config.base_url + "/path/with/_regex_/";
    EXPECT_EQ(
        stats
            .SingleMetric(
                "test.reply-statuses",
                {{"http_destination", expected_destination_name}, {"http_code", "200"}}
            )
            .AsRate(),
        1
    );
}

UTEST(RequestsMultipleContentTypes, Json) {
    const utest::HttpServerMock http_server([&](const utest::HttpServerMock::HttpRequest& request) {
        EXPECT_EQ(request.body, R"({"foo":"a"})");
        EXPECT_EQ(request.headers.at(std::string{"Content-Type"}), "application/json");
        return utest::HttpServerMock::HttpResponse{200};
    });

    auto http_client_ptr = utest::CreateHttpClient();
    auto request = http_client_ptr->CreateRequest();

    client::SerializeRequest({client::RequestBodyApplicationJson{"a"}}, http_server.GetBaseUrl(), request);

    auto response = request.perform();
    EXPECT_EQ(response->status_code(), 200);
}

UTEST(RequestsMultipleContentTypes, XWwwFormUrlencoded) {
    const utest::HttpServerMock http_server([&](const utest::HttpServerMock::HttpRequest& request) {
        EXPECT_EQ(request.body, "is_smoking=true&salary=1000.500000&age=30&password=123%20456&name=abc");
        EXPECT_EQ(request.headers.at(std::string{"Content-Type"}), "application/x-www-form-urlencoded");
        return utest::HttpServerMock::HttpResponse{200};
    });

    auto http_client_ptr = utest::CreateHttpClient();
    auto request = http_client_ptr->CreateRequest();

    client::SerializeRequest(
        {client::RequestBodyApplicationXWwwFormUrlencoded{"abc", "123 456", 30, 1000.5, true}},
        http_server.GetBaseUrl(),
        request
    );

    auto response = request.perform();
    EXPECT_EQ(response->status_code(), 200);
}

UTEST(RequestsMultipleContentTypes, MultipartFormData) {
    const utest::HttpServerMock http_server([&](const utest::HttpServerMock::HttpRequest& request) {
        const http::ContentType content_type(request.headers.at(std::string{"Content-Type"}));
        EXPECT_EQ(content_type.MediaType(), "multipart/form-data");
        const auto& boundary = content_type.Boundary();
        EXPECT_EQ(
            request.body,
            "--" + boundary +
                "\r\n"
                "Content-Disposition: form-data; name=\"filename\"\r\n"
                "\r\nfilename\r\n" +
                "--" + boundary +
                "\r\n"
                "Content-Disposition: form-data; name=\"content\"\r\n"
                "\r\nfile\ncontent\r\n" +
                "--" + boundary + "--\r\n"
        );
        return utest::HttpServerMock::HttpResponse{200};
    });

    auto http_client_ptr = utest::CreateHttpClient();
    auto request = http_client_ptr->CreateRequest();

    client::SerializeRequest(
        {client::RequestBodyMultipartFormData{"filename", "file\ncontent"}},
        http_server.GetBaseUrl(),
        request
    );

    auto response = request.perform();
    EXPECT_EQ(response->status_code(), 200);
}

UTEST(RequestsMultipleContentTypes, OctetStream) {
    const utest::HttpServerMock http_server([&](const utest::HttpServerMock::HttpRequest& request) {
        EXPECT_EQ(request.body, "blabla");
        EXPECT_EQ(request.headers.at(std::string{"Content-Type"}), "application/octet-stream");
        return utest::HttpServerMock::HttpResponse{200};
    });

    auto http_client_ptr = utest::CreateHttpClient();
    auto request = http_client_ptr->CreateRequest();

    client::SerializeRequest({client::RequestBodyApplicationOctetStream{"blabla"}}, http_server.GetBaseUrl(), request);

    auto response = request.perform();
    EXPECT_EQ(response->status_code(), 200);
}

class RequestsQueryLogMode : public utest::LogCaptureFixture<> {};

UTEST_F(RequestsQueryLogMode, HideOperation) {
    const utest::HttpServerMock http_server([&](const utest::HttpServerMock::HttpRequest&) {
        return utest::HttpServerMock::HttpResponse{200};
    });
    auto http_client_ptr = utest::CreateHttpClient();
    auto request = http_client_ptr->CreateRequest();

    namespace client = ::clients::parameters::test1_query_log_mode::get;
    client::SerializeRequest({client::Request{"foo", "bar"}}, http_server.GetBaseUrl(), request);
    auto response = request.perform();

    EXPECT_EQ(response->status_code(), 200);

    auto text = GetLogCapture().GetAll().back().GetTag("url.full");
    EXPECT_TRUE(utils::text::EndsWith(text, "test1/query-log-mode?password=***&secret=***"));
}

UTEST_F(RequestsQueryLogMode, HideParameter) {
    const utest::HttpServerMock http_server([&](const utest::HttpServerMock::HttpRequest&) {
        return utest::HttpServerMock::HttpResponse{200};
    });
    auto http_client_ptr = utest::CreateHttpClient();
    auto request = http_client_ptr->CreateRequest();

    namespace client = ::clients::parameters::test1_query_log_mode_parameter::get;
    client::SerializeRequest({client::Request{"foo", "bar"}}, http_server.GetBaseUrl(), request);
    auto response = request.perform();

    EXPECT_EQ(response->status_code(), 200);

    auto text = GetLogCapture().GetAll().back().GetTag("url.full");
    EXPECT_TRUE(utils::text::EndsWith(text, "test1/query-log-mode/parameter?password=***&secret=bar"));
}

}  // namespace

USERVER_NAMESPACE_END
