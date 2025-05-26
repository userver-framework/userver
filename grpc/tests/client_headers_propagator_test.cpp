#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/server/request/task_inherited_request.hpp>

#include <userver/ugrpc/client/middlewares/headers_propagator/middleware.hpp>
#include <userver/ugrpc/impl/to_string.hpp>
#include <userver/utils/text.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class UnitTestServiceMock : public sample::ugrpc::UnitTestServiceBase {
public:
    MOCK_METHOD(
        SayHelloResult,
        SayHello,
        (ugrpc::server::CallContext& /*context*/, ::sample::ugrpc::GreetingRequest&& /*request*/),
        (override)
    );
};

class GrpcClientHeadersPropagator : public ugrpc::tests::ServiceFixtureBase {
public:
    GrpcClientHeadersPropagator() {
        SetClientMiddlewares({std::make_shared<ugrpc::client::middlewares::headers_propagator::Middleware>()});
        RegisterService(service_);
        StartServer();
    };

    ~GrpcClientHeadersPropagator() override { StopServer(); }

    UnitTestServiceMock& Service() { return service_; }

private:
    UnitTestServiceMock service_;
};

}  // namespace

UTEST_F(GrpcClientHeadersPropagator, Propagate) {
    boost::container::small_vector<USERVER_NAMESPACE::server::request::Header, 2> headers{
        {"UPPER", "v1"},
        {"lower", "v2"},
    };
    USERVER_NAMESPACE::server::request::SetPropagatedHeaders({headers[0], headers[1]});
    static const std::string kNull = "null";

    ON_CALL(Service(), SayHello)
        .WillByDefault([&headers](ugrpc::server::CallContext& context, ::sample::ugrpc::GreetingRequest&&) {
            auto& client_metadata = context.GetServerContext().client_metadata();
            EXPECT_GT(headers.size(), 0);
            EXPECT_GE(client_metadata.size(), headers.size());

            for (const auto& [name, value] : headers) {
                const auto it = client_metadata.find(ugrpc::impl::ToGrpcStringRef(utils::text::ToLower(name)));
                EXPECT_TRUE(it != client_metadata.end());
                EXPECT_EQ(ugrpc::impl::ToGrpcStringRef(value), it->second);
            }

            sample::ugrpc::GreetingResponse response;
            response.set_name(kNull);
            return response;
        });

    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    sample::ugrpc::GreetingRequest out;
    auto context = std::make_unique<grpc::ClientContext>();

    sample::ugrpc::GreetingResponse in;
    UEXPECT_NO_THROW(in = client.SayHello(out, std::move(context)));
    ASSERT_EQ(in.name(), kNull);
}

USERVER_NAMESPACE_END
