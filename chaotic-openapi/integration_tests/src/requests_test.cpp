#include <userver/utest/utest.hpp>

#include <gmock/gmock.h>

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

using namespace ::clients::multiple_content_types;

class RequestsMultipleContentTypes : public ::testing::Test {
protected: 
    template <typename Callback>
    void SetupCallback(Callback&& callback){
        mock_server_ = std::make_unique<utest::HttpServerMock>(
            [hook = std::move(callback)](const utest::HttpServerMock::HttpRequest& request) {
                hook(request);
                utest::HttpServerMock::HttpResponse response{};
                response.response_status = 200;
                return response;
            });
    }

    template <typename Request>
    void PerformRequest(Request&& request_obj) {
        EXPECT_NE(mock_server_.get(), nullptr);
        auto http_client_ptr = utest::CreateHttpClient();
        auto request = http_client_ptr->CreateRequest();
        SerializeRequest(std::move(request_obj), mock_server_->GetBaseUrl(), request);
        auto response = request.perform();
        EXPECT_EQ(response->status_code(), 200);
    }

private:
    std::unique_ptr<utest::HttpServerMock> mock_server_;
};

UTEST(Requests, RegexDestinationName) {
    const utest::HttpServerMock http_server([&](const utest::HttpServerMock::HttpRequest&) {
        utest::HttpServerMock::HttpResponse response{};
        response.response_status = 200;
        return response;
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

UTEST_F(RequestsMultipleContentTypes, Json) {
    SetupCallback([](const utest::HttpServerMock::HttpRequest& request) {
            EXPECT_EQ(request.body, R"({"foo":"a"})");
            EXPECT_EQ(request.headers.at(std::string{"Content-Type"}), "application/json");
        });
    const auto& json_obj = single_json{"a"};
    PerformRequest(test1::post::Request{json_obj});
    PerformRequest(test_single_json::post::Request{json_obj});
}

UTEST_F(RequestsMultipleContentTypes, XWwwFormUrlencoded) {
    SetupCallback([](const utest::HttpServerMock::HttpRequest& request) {
            // x-www-form-urlencoded field order is unspecified (serialized from a
            // std::unordered_map), so compare the '&'-separated parts order-independently.
            const auto parts = utils::text::Split(request.body, "&");
            EXPECT_THAT(
                parts,
                ::testing::UnorderedElementsAre(
                    "name=abc",
                    "password=123%20456",
                    "age=30",
                    "salary=1000.500000",
                    "is_smoking=true"
                )
            );
            EXPECT_EQ(request.headers.at(std::string{"Content-Type"}), "application/x-www-form-urlencoded");
        });
    const auto& form_urlen_obj = single_form_urlen{"abc", "123 456", 30, 1000.5, true};        
    PerformRequest(test1::post::Request{form_urlen_obj});
    PerformRequest(test_single_form_urlen::post::Request{form_urlen_obj});
}


UTEST_F(RequestsMultipleContentTypes, MultipartFormData) {
    SetupCallback([](const utest::HttpServerMock::HttpRequest& request) {
            const auto& raw_content_type = request.headers.at(std::string{"Content-Type"});
            const http::ContentType content_type(raw_content_type);
            EXPECT_EQ(content_type.MediaType(), "multipart/form-data");
            const auto& boundary = content_type.Boundary();
            EXPECT_THAT(raw_content_type, ::testing::HasSubstr("boundary="));
            EXPECT_FALSE(boundary.empty());
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
        });
    const auto& form_data_obj = single_form_data{"filename", "file\ncontent"};
    PerformRequest(test1::post::Request{form_data_obj});
    PerformRequest(test_single_form_data::post::Request{form_data_obj});
}

UTEST_F(RequestsMultipleContentTypes, OctetStream) {
    SetupCallback([](const utest::HttpServerMock::HttpRequest& request) {
            EXPECT_EQ(request.body, "blabla");
            EXPECT_EQ(request.headers.at(std::string{"Content-Type"}), "application/octet-stream");
    });
    const auto& single_octet_obj = single_octet("blabla");
    PerformRequest(test1::post::Request{test1::post::RequestBodyApplicationOctetStream{single_octet_obj}});
    PerformRequest(test_single_octet::post::Request{single_octet_obj});
}

class RequestsQueryLogMode : public utest::LogCaptureFixture<> {};

UTEST_F(RequestsQueryLogMode, HideOperation) {
    const utest::HttpServerMock http_server([&](const utest::HttpServerMock::HttpRequest&) {
        utest::HttpServerMock::HttpResponse response{};
        response.response_status = 200;
        return response;
    });
    auto http_client_ptr = utest::CreateHttpClient();
    auto request = http_client_ptr->CreateRequest();

    namespace client = ::clients::parameters::test1_query_log_mode::get;
    client::SerializeRequest({client::Request{"foo", "bar"}}, http_server.GetBaseUrl(), request);
    auto response = request.perform();

    EXPECT_EQ(response->status_code(), 200);

    auto text = GetLogCapture().GetAll().back().GetTag("url.full");
    EXPECT_TRUE(text.ends_with("test1/query-log-mode?password=***&secret=***"));
}

UTEST_F(RequestsQueryLogMode, HideParameter) {
    const utest::HttpServerMock http_server([&](const utest::HttpServerMock::HttpRequest&) {
        utest::HttpServerMock::HttpResponse response{};
        response.response_status = 200;
        return response;
    });
    auto http_client_ptr = utest::CreateHttpClient();
    auto request = http_client_ptr->CreateRequest();

    namespace client = ::clients::parameters::test1_query_log_mode_parameter::get;
    client::SerializeRequest({client::Request{"foo", "bar"}}, http_server.GetBaseUrl(), request);
    auto response = request.perform();

    EXPECT_EQ(response->status_code(), 200);

    auto text = GetLogCapture().GetAll().back().GetTag("url.full");
    EXPECT_TRUE(text.ends_with("test1/query-log-mode/parameter?password=***&secret=bar"));
}

}  // namespace

USERVER_NAMESPACE_END
