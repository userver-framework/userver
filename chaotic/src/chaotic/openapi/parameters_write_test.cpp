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

UTEST(OpenapiParameters, Cookie) {
    static constexpr co::Name kName{"test"};

    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetCookie("test", std::string{"value"}));

    co::WriteParameter<co::TrivialParameter<co::In::kCookie, kName, std::string>>("value", sink);
}

UTEST(OpenapiParameters, Path) {
    static constexpr co::Name kName{"test"};

    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetPath("test", std::string{"value"}));

    co::WriteParameter<co::TrivialParameter<co::In::kPath, kName, std::string>>("value", sink);
}

UTEST(OpenapiParameters, Header) {
    static constexpr co::Name kName{"test"};

    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetHeader("test", std::string{"value"}));

    co::WriteParameter<co::TrivialParameter<co::In::kHeader, kName, std::string>>("value", sink);
}

UTEST(OpenapiParameters, Query) {
    static constexpr co::Name kName{"test"};

    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetQuery("test", std::string{"value"}));

    co::WriteParameter<co::TrivialParameter<co::In::kQuery, kName, std::string>>("value", sink);
}

UTEST(OpenapiParameters, QueryExplode) {
    static constexpr co::Name kName{"test"};

    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetMultiQuery("test", (std::vector<std::string>{"foo", "bar"})));

    co::WriteParameter<co::ArrayParameter<co::In::kQueryExplode, kName, ',', std::string>>(
        std::vector<std::string>{"foo", "bar"}, sink
    );
}

UTEST(OpenapiParameters, QueryExplodeInteger) {
    static constexpr co::Name kName{"test"};

    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetMultiQuery("test", (std::vector<std::string>{"1", "2"})));

    co::WriteParameter<co::ArrayParameter<co::In::kQueryExplode, kName, ',', int>>(std::vector<int>{1, 2}, sink);
}

UTEST(OpenapiParameters, QueryExplodeUser) {
    static constexpr co::Name kName{"test"};

    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetMultiQuery("test", (std::vector<std::string>{"1.2", "3.4"})));

    using Decimal = decimal64::Decimal<10>;
    co::WriteParameter<co::ArrayParameter<co::In::kQueryExplode, kName, ',', std::string, Decimal>>(
        std::vector<Decimal>{Decimal{"1.2"}, Decimal{"3.4"}}, sink
    );
}

UTEST(OpenapiParameters, CookieArray) {
    static constexpr co::Name kName{"test"};

    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetCookie("test", std::string{"foo,bar"}));

    co::WriteParameter<co::ArrayParameter<co::In::kCookie, kName, ',', std::string, std::string>>({"foo", "bar"}, sink);
}

UTEST(OpenapiParameters, QueryArrayOfInteger) {
    static constexpr co::Name kName{"test"};

    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetQuery("test", std::string{"1,2"}));

    co::WriteParameter<co::ArrayParameter<co::In::kQuery, kName, ',', int>>({1, 2}, sink);
}

UTEST(OpenapiParameters, QueryArrayOfUserTypes) {
    static constexpr co::Name kName{"test"};

    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetQuery("test", std::string{"1.1,2.2"}));

    using Decimal = decimal64::Decimal<10>;
    co::WriteParameter<co::ArrayParameter<co::In::kQuery, kName, ',', std::string, Decimal>>(
        {Decimal{"1.1"}, Decimal{"2.2"}}, sink
    );
}

UTEST(OpenapiParameters, UserType) {
    static constexpr co::Name kName{"test"};

    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetCookie("test", std::string{"1.1"}));

    using Decimal = decimal64::Decimal<10>;
    co::WriteParameter<co::TrivialParameter<co::In::kCookie, kName, std::string, Decimal>>(Decimal{"1.1"}, sink);
}

UTEST(OpenapiParameters, TypeBoolean) {
    static constexpr co::Name kName{"test"};

    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetCookie("test", std::string{"true"}));

    bool bool_var = true;
    co::WriteParameter<co::TrivialParameter<co::In::kCookie, kName, bool>>(bool_var, sink);
}

UTEST(OpenapiParameters, TypeDouble) {
    static constexpr co::Name kName{"test"};

    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetCookie("test", std::string{"2.1"}));

    double double_var = 2.1;
    co::WriteParameter<co::TrivialParameter<co::In::kCookie, kName, double>>(double_var, sink);
}

UTEST(OpenapiParameters, TypeInt) {
    static constexpr co::Name kName{"test"};

    ParameterSinkMock sink;
    EXPECT_CALL(sink, SetCookie("test", std::string{"1"}));
    int int_var = 1;
    co::WriteParameter<co::TrivialParameter<co::In::kCookie, kName, int>>(int_var, sink);
}

UTEST(OpenapiParameters, SinkHttpClient) {
    auto http_client_ptr = utest::CreateHttpClient();
    auto request = http_client_ptr->CreateRequest();
    bool called = false;
    utest::HttpServerMock http_server([&called](const utest::HttpServerMock::HttpRequest& request) {
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

    static constexpr co::Name kName{
        "name",
    };

    co::ParameterSinkHttpClient sink(request, http_server.GetBaseUrl() + "/{name}");

    static constexpr co::Name kVar1{
        "var1",
    };
    static constexpr co::Name kVar2{
        "var2",
    };
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
    static constexpr co::Name kName{"test"};

    EXPECT_THROW(
        (co::WriteParameter<co::TrivialParameter<co::In::kPath, kName, std::string, std::string>>("foo?bar", sink)),
        std::runtime_error
    );

    EXPECT_THROW(
        (co::WriteParameter<co::TrivialParameter<co::In::kPath, kName, std::string, std::string>>("foo/bar", sink)),
        std::runtime_error
    );
}

USERVER_NAMESPACE_END
