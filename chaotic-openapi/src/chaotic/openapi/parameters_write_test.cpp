#include <userver/utest/utest.hpp>

#include <gmock/gmock.h>

#include <userver/chaotic/io/userver/decimal64/decimal.hpp>
#include <userver/chaotic/openapi/parameters_write.hpp>
#include <userver/clients/http/client.hpp>
#include <userver/engine/run_in_coro.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/http_server_mock.hpp>

USERVER_NAMESPACE_BEGIN

namespace co = chaotic::openapi;

class ParameterSinkMock : public co::ParameterSinkBase {
public:
    MOCK_METHOD(void, SetCookie, (std::string_view, std::string&&), (override));

    MOCK_METHOD(void, SetHeader, (std::string_view, std::string&&), (override));

    MOCK_METHOD(void, SetPath, (co::Name&, std::string&&), (override));

    MOCK_METHOD(void, SetQuery, (std::string_view, std::string&&), (override));

    MOCK_METHOD(void, SetMultiQuery, (std::string_view, std::vector<std::string>&&), (override));
};

static constexpr co::Name kTest{
    "test",
};

UTEST(OpenapiParameters, Cookie) {
    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetCookie(std::string_view{"test"}, std::string{"value"}));

    co::WriteParameter<co::TrivialParameter<co::In::kCookie, kTest, std::string>>("value", sink);
}

UTEST(OpenapiParameters, Path) {
    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetPath("test", std::string{"value"}));

    co::WriteParameter<co::TrivialParameter<co::In::kPath, kTest, std::string>>("value", sink);
}

UTEST(OpenapiParameters, Header) {
    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetHeader(std::string_view{"test"}, std::string{"value"}));

    co::WriteParameter<co::TrivialParameter<co::In::kHeader, kTest, std::string>>("value", sink);
}

UTEST(OpenapiParameters, Query) {
    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetQuery(std::string_view{"test"}, std::string{"value"}));

    co::WriteParameter<co::TrivialParameter<co::In::kQuery, kTest, std::string>>("value", sink);
}

UTEST(OpenapiParameters, QueryExplode) {
    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetMultiQuery(std::string_view{"test"}, (std::vector<std::string>{"foo", "bar"})));

    co::WriteParameter<co::ArrayParameter<co::In::kQueryExplode, kTest, ',', std::string>>(
        std::vector<std::string>{"foo", "bar"}, sink
    );
}

UTEST(OpenapiParameters, QueryExplodeInteger) {
    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetMultiQuery(std::string_view{"test"}, (std::vector<std::string>{"1", "2"})));

    co::WriteParameter<co::ArrayParameter<co::In::kQueryExplode, kTest, ',', int>>(std::vector<int>{1, 2}, sink);
}

UTEST(OpenapiParameters, QueryExplodeUser) {
    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetMultiQuery(std::string_view{"test"}, (std::vector<std::string>{"1.2", "3.4"})));

    using Decimal = decimal64::Decimal<10>;
    co::WriteParameter<co::ArrayParameter<co::In::kQueryExplode, kTest, ',', std::string, Decimal>>(
        std::vector<Decimal>{Decimal{"1.2"}, Decimal{"3.4"}}, sink
    );
}

UTEST(OpenapiParameters, CookieArray) {
    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetCookie(std::string_view{"test"}, std::string{"foo,bar"}));

    co::WriteParameter<co::ArrayParameter<co::In::kCookie, kTest, ',', std::string, std::string>>({"foo", "bar"}, sink);
}

UTEST(OpenapiParameters, QueryArrayOfInteger) {
    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetQuery(std::string_view{"test"}, std::string{"1,2"}));

    co::WriteParameter<co::ArrayParameter<co::In::kQuery, kTest, ',', int>>({1, 2}, sink);
}

UTEST(OpenapiParameters, QueryArrayOfUserTypes) {
    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetQuery(std::string_view{"test"}, std::string{"1.1,2.2"}));

    using Decimal = decimal64::Decimal<10>;
    co::WriteParameter<co::ArrayParameter<co::In::kQuery, kTest, ',', std::string, Decimal>>(
        {Decimal{"1.1"}, Decimal{"2.2"}}, sink
    );
}

UTEST(OpenapiParameters, UserType) {
    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetCookie(std::string_view{"test"}, std::string{"1.1"}));

    using Decimal = decimal64::Decimal<10>;
    co::WriteParameter<co::TrivialParameter<co::In::kCookie, kTest, std::string, Decimal>>(Decimal{"1.1"}, sink);
}

UTEST(OpenapiParameters, TypeBoolean) {
    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetCookie(std::string_view{"test"}, std::string{"true"}));

    const bool bool_var = true;
    co::WriteParameter<co::TrivialParameter<co::In::kCookie, kTest, bool>>(bool_var, sink);
}

UTEST(OpenapiParameters, TypeDouble) {
    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetCookie(std::string_view{"test"}, std::string{"2.1"}));

    const double double_var = 2.1;
    co::WriteParameter<co::TrivialParameter<co::In::kCookie, kTest, double>>(double_var, sink);
}

UTEST(OpenapiParameters, TypeInt) {
    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetCookie(std::string_view{"test"}, std::string{"1"}));
    const int int_var = 1;
    co::WriteParameter<co::TrivialParameter<co::In::kCookie, kTest, int>>(int_var, sink);
}

static constexpr co::Name kName{
    "name",
};

static constexpr co::Name kVar1{
    "var1",
};
static constexpr co::Name kVar2{
    "var2",
};

UTEST(OpenapiParameters, SinkHttpClient) {
    auto http_client_ptr = utest::CreateHttpClient();
    auto request = http_client_ptr->CreateRequest();
    bool called = false;
    const utest::HttpServerMock http_server([&called](const utest::HttpServerMock::HttpRequest& request) {
        constexpr http::headers::PredefinedHeader kName("name");
        constexpr http::headers::PredefinedHeader kCookie("cookie");

        EXPECT_EQ(request.path, "/foo4");
        EXPECT_EQ(
            request.query, (std::multimap<std::string, std::string>{{"var1", "foo"}, {"var1", "bar"}, {"var2", "foo1"}})
        );
        EXPECT_EQ(request.headers.at(kName), "foo2");
        EXPECT_EQ(request.headers.at(kCookie), "name=foo3");
        called = true;

        return utest::HttpServerMock::HttpResponse{};
    });

    co::ParameterSinkHttpClient sink(request, http_server.GetBaseUrl() + "/{name}");

    co::WriteParameter<co::ArrayParameter<co::In::kQueryExplode, kVar1, ',', std::string>>(
        std::vector<std::string>{"foo", "bar"}, sink
    );
    co::WriteParameter<co::TrivialParameter<co::In::kQuery, kVar2, std::string>>("foo1", sink);

    co::WriteParameter<co::TrivialParameter<co::In::kHeader, kName, std::string>>("foo2", sink);
    co::WriteParameter<co::TrivialParameter<co::In::kCookie, kName, std::string>>("foo3", sink);
    co::WriteParameter<co::TrivialParameter<co::In::kPath, kName, std::string>>("foo4", sink);
    sink.Flush();

    auto response = request.perform();
    EXPECT_TRUE(called);
}

UTEST(OpenapiParameters, InvalidPathVariable) {
    ParameterSinkMock sink;

    EXPECT_THROW(
        (co::WriteParameter<co::TrivialParameter<co::In::kPath, kTest, std::string, std::string>>("foo?bar", sink)),
        std::runtime_error
    );

    EXPECT_THROW(
        (co::WriteParameter<co::TrivialParameter<co::In::kPath, kTest, std::string, std::string>>("foo/bar", sink)),
        std::runtime_error
    );
}

USERVER_NAMESPACE_END
