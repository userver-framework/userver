#include <userver/utest/utest.hpp>

#include <type_traits>

#include <gmock/gmock.h>

#include <userver/chaotic/exception.hpp>
#include <userver/clients/http/client.hpp>
#include <userver/clients/http/client_core.hpp>
#include <userver/dump/operations_mock.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
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
#include <clients/test_object/requests.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace client = ::clients::multiple_content_types::test1::post;
namespace test_object = ::clients::test_object;

template <typename T>
std::string SerializeViaDom(const T& value) {
    return formats::json::ToString(formats::json::ValueBuilder(value).ExtractValue());
}

test_object::SerializationObject MakeSerializationObject() {
    test_object::SerializationObject value;
    value.required_field = "required";
    value.optional_field = 11;
    value.nullable_field = "nullable";
    value.nested.label = "nested";
    value.numbers = {1, 2};
    value.custom_number = 3;
    return value;
}

static_assert(std::is_same_v<test_object::test1::post::Body, test_object::SerializationObject>);
static_assert(std::is_same_v<decltype(test_object::SerializationObject::custom_number), unsigned>);

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

UTEST(RequestsMultipleContentTypes, Json) {
    const utest::HttpServerMock http_server([&](const utest::HttpServerMock::HttpRequest& request) {
        EXPECT_EQ(request.body, R"({"foo":"a"})");
        EXPECT_EQ(request.headers.at(std::string{"Content-Type"}), "application/json");
        utest::HttpServerMock::HttpResponse response{};
        response.response_status = 200;
        return response;
    });

    auto http_client_ptr = utest::CreateHttpClient();
    auto request = http_client_ptr->CreateRequest();

    client::SerializeRequest({client::RequestBodyApplicationJson{"a"}}, http_server.GetBaseUrl(), request);

    auto response = request.perform();
    EXPECT_EQ(response->status_code(), 200);
}

UTEST(RequestSerializationDom, GeneratedObjectExactBody) {
    const auto value = MakeSerializationObject();

    EXPECT_EQ(
        SerializeViaDom(value),
        R"({"required-field":"required","optional-field":11,"nullable-field":"nullable","defaulted-field":7,"nested":{"label":"nested"},"numbers":[1,2],"custom-number":3})"
    );
}

UTEST(RequestSerializationDom, GeneratedObjectIsWrittenToHttpBody) {
    const auto
        expected =
            R"({"required-field":"required","optional-field":11,"nullable-field":"nullable","defaulted-field":7,"nested":{"label":"nested"},"numbers":[1,2],"custom-number":3})";
    const utest::HttpServerMock http_server([&](const utest::HttpServerMock::HttpRequest& request) {
        EXPECT_EQ(request.body, expected);
        utest::HttpServerMock::HttpResponse response{};
        response.response_status = 201;
        return response;
    });

    auto http_client = utest::CreateHttpClient();
    auto http_request = http_client->CreateRequest();
    test_object::test1::post::SerializeRequest({MakeSerializationObject()}, http_server.GetBaseUrl(), http_request);

    const auto response = http_request.perform();
    EXPECT_EQ(response->status_code(), 201);
}

UTEST(RequestSerializationDom, OptionalFieldIsOmitted) {
    auto value = MakeSerializationObject();
    value.optional_field.reset();

    EXPECT_EQ(SerializeViaDom(value), R"({"required-field":"required","nullable-field":"nullable","defaulted-field":7,"nested":{"label":"nested"},"numbers":[1,2],"custom-number":3})");
}

UTEST(RequestSerializationDom, NullNullableFieldIsOmitted) {
    auto value = MakeSerializationObject();
    value.nullable_field.reset();

    EXPECT_EQ(SerializeViaDom(value), R"({"required-field":"required","optional-field":11,"defaulted-field":7,"nested":{"label":"nested"},"numbers":[1,2],"custom-number":3})");
}

UTEST(RequestSerializationDom, AdditionalPropertiesJsonDeclaredFieldsWin) {
    test_object::AdditionalPropertiesJson value;
    value.required_field = "declared-required";
    value.optional_field = "declared-optional";
    value.extra = formats::json::FromString(
        R"({"before":1,"required-field":"extra-required","optional-field":"extra-optional","after":2})"
    );

    EXPECT_EQ(
        SerializeViaDom(value),
        R"({"before":1,"required-field":"declared-required","optional-field":"declared-optional","after":2})"
    );
}

UTEST(RequestSerializationDom, AdditionalPropertiesJsonAbsentOptionalKeepsExtra) {
    test_object::AdditionalPropertiesJson value;
    value.required_field = "declared-required";
    value.extra = formats::json::FromString(R"({"optional-field":"extra-optional"})");

    EXPECT_EQ(SerializeViaDom(value), R"({"optional-field":"extra-optional","required-field":"declared-required"})");
}

UTEST(RequestSerializationDom, AdditionalPropertiesTypedDeclaredFieldWins) {
    test_object::AdditionalPropertiesTyped value;
    value.required_field = 3;
    value.extra = {{"required-field", 2}};

    EXPECT_EQ(SerializeViaDom(value), R"({"required-field":3})");
}

UTEST(RequestSerializationDom, AdditionalPropertiesTypedAbsentOptionalKeepsExtra) {
    test_object::AdditionalPropertiesTyped value;
    value.required_field = 3;
    value.extra = {{"optional-field", 4}};

    EXPECT_EQ(SerializeViaDom(value), R"({"optional-field":4,"required-field":3})");
}

UTEST(RequestSerializationDom, SafeAllOfExactBody) {
    test_object::SafeAllOf value;
    value.base_field = "base";
    value.child_field = 5;

    EXPECT_EQ(SerializeViaDom(value), R"({"base-field":"base","child-field":5})");
}

UTEST(RequestSerializationDom, OverlappingAllOfUsesLastParentValue) {
    test_object::RiskyAllOf value;
    static_cast<test_object::RiskyAllOfBase&>(value).overlapping_field = "base";
    static_cast<test_object::RiskyAllOf__P1&>(value).overlapping_field = "last";

    EXPECT_EQ(SerializeViaDom(value), R"({"overlapping-field":"last"})");
}

UTEST(RequestSerializationDom, SerializationDoesNotRunValidators) {
    test_object::ValidatedObject value;
    value.count = 0;

    EXPECT_EQ(SerializeViaDom(value), R"({"count":0})");
    UEXPECT_THROW_MSG(
        formats::json::FromString(R"({"count":0})").As<test_object::ValidatedObject>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'count': Invalid value, minimum=1, given=0"
    );
}

UTEST(RequestsMultipleContentTypes, XWwwFormUrlencoded) {
    const utest::HttpServerMock http_server([&](const utest::HttpServerMock::HttpRequest& request) {
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
        utest::HttpServerMock::HttpResponse response{};
        response.response_status = 200;
        return response;
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
        utest::HttpServerMock::HttpResponse response{};
        response.response_status = 200;
        return response;
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
        utest::HttpServerMock::HttpResponse response{};
        response.response_status = 200;
        return response;
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
