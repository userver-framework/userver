#include <gtest/gtest.h>

#include <string_view>

#include <userver/baggage/baggage_manager.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/impl/userver_experiments.hpp>

#include <../include/userver/ugrpc/client/impl/completion_queue_pool.hpp>
#include <ugrpc/client/impl/client_configs.hpp>
#include <ugrpc/client/middlewares/baggage/middleware.hpp>
#include <ugrpc/impl/rpc_metadata.hpp>
#include <ugrpc/server/impl/server_configs.hpp>
#include <ugrpc/server/middlewares/baggage/middleware.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/impl/to_string.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class ServerBaggageTestService final : public sample::ugrpc::UnitTestServiceBase {
public:
    void SayHello(SayHelloCall& call, sample::ugrpc::GreetingRequest&&) override {
        sample::ugrpc::GreetingResponse response;
        const auto* bg = baggage::BaggageManager::TryGetBaggage();

        if (bg) {
            response.set_name(bg->ToString());
        } else {
            response.set_name("null");
        }

        call.Finish(response);
    }
};

class GrpcServerTestBaggage : public ugrpc::tests::ServiceFixtureBase {
public:
    GrpcServerTestBaggage() {
        SetServerMiddlewares({std::make_shared<ugrpc::server::middlewares::baggage::Middleware>()});

        ExtendDynamicConfig({
            {baggage::kBaggageSettings, {{"key1", "key2", "key3"}}},
            {baggage::kBaggageEnabled, true},
        });

        RegisterService(service_);
        StartServer();
    }

    ~GrpcServerTestBaggage() override { StopServer(); }

private:
    ServerBaggageTestService service_;
};

}  // namespace

UTEST_F(GrpcServerTestBaggage, TestGrpcBaggage) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    sample::ugrpc::GreetingRequest out;
    auto context = std::make_unique<grpc::ClientContext>();
    const std::string baggage = "key1=value1";

    context->AddMetadata(ugrpc::impl::kXBaggage, ugrpc::impl::ToGrpcString(baggage));
    auto call = client.SayHello(out, std::move(context));

    sample::ugrpc::GreetingResponse in;
    UEXPECT_NO_THROW(in = call.Finish());
    ASSERT_EQ(in.name(), baggage);
}

UTEST_F(GrpcServerTestBaggage, TestGrpcBaggageMultiply) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    sample::ugrpc::GreetingRequest out;
    auto context = std::make_unique<grpc::ClientContext>();

    const std::string baggage = "key1=value1;key2=value2;key3";
    context->AddMetadata(ugrpc::impl::kXBaggage, ugrpc::impl::ToGrpcString(baggage));
    auto call = client.SayHello(out, std::move(context));

    sample::ugrpc::GreetingResponse in;
    UEXPECT_NO_THROW(in = call.Finish());
    ASSERT_EQ(in.name(), baggage);
}

UTEST_F(GrpcServerTestBaggage, TestGrpcBaggageNoBaggage) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    sample::ugrpc::GreetingRequest out;
    auto context = std::make_unique<grpc::ClientContext>();

    auto call = client.SayHello(out, std::move(context));

    sample::ugrpc::GreetingResponse in;
    UEXPECT_NO_THROW(in = call.Finish());
    ASSERT_EQ(in.name(), "null");
}

UTEST_F(GrpcServerTestBaggage, TestGrpcBaggageWrongKey) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    sample::ugrpc::GreetingRequest out;
    auto context = std::make_unique<grpc::ClientContext>();

    context->AddMetadata(ugrpc::impl::kXBaggage, "wrong_key=wrong_value");
    auto call = client.SayHello(out, std::move(context));

    sample::ugrpc::GreetingResponse in;
    UEXPECT_NO_THROW(in = call.Finish());
    ASSERT_EQ(in.name(), "");
}

namespace {

class ClientBaggageTestService final : public sample::ugrpc::UnitTestServiceBase {
public:
    void SayHello(SayHelloCall& call, sample::ugrpc::GreetingRequest&&) override {
        sample::ugrpc::GreetingResponse response;
        const auto* baggage_header = utils::FindOrNullptr(call.GetContext().client_metadata(), ugrpc::impl::kXBaggage);

        if (baggage_header) {
            response.set_name(ugrpc::impl::ToString(*baggage_header));
        } else {
            response.set_name("null");
        }

        call.Finish(response);
    }
};

class GrpcClientTestBaggage : public ugrpc::tests::ServiceFixtureBase {
public:
    GrpcClientTestBaggage() {
        ExtendDynamicConfig({
            {baggage::kBaggageSettings, {{"key1", "key2", "key3"}}},
            {baggage::kBaggageEnabled, true},
        });
        SetClientMiddlewareFactories({std::make_shared<ugrpc::client::middlewares::baggage::MiddlewareFactory>()});
        RegisterService(service_);
        StartServer();
    };

    ~GrpcClientTestBaggage() override { StopServer(); }

    baggage::BaggageManager& BaggageManager() { return baggage_manager_; }

private:
    ClientBaggageTestService service_;
    baggage::BaggageManager baggage_manager_ = baggage::BaggageManager(GetConfigSource());
};

}  // namespace

UTEST_F(GrpcClientTestBaggage, TestGrpcClientBaggage) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    const std::string baggage = "key1=value1";

    sample::ugrpc::GreetingRequest request;

    BaggageManager().SetBaggage(baggage);

    auto context = std::make_unique<grpc::ClientContext>();
    auto call = client.SayHello(request, std::move(context));

    sample::ugrpc::GreetingResponse in;
    UEXPECT_NO_THROW(in = call.Finish());
    ASSERT_EQ(in.name(), baggage);
}

UTEST_F(GrpcClientTestBaggage, TestGrpcClientBaggageMultiply) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    const std::string baggage = "key1=value1;key2=value2;key3";

    sample::ugrpc::GreetingRequest request;

    BaggageManager().SetBaggage(baggage);

    auto context = std::make_unique<grpc::ClientContext>();
    auto call = client.SayHello(request, std::move(context));

    sample::ugrpc::GreetingResponse in;
    UEXPECT_NO_THROW(in = call.Finish());
    ASSERT_EQ(in.name(), baggage);
}

UTEST_F(GrpcClientTestBaggage, TestGrpcClientNoBaggage) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

    sample::ugrpc::GreetingRequest request;

    auto context = std::make_unique<grpc::ClientContext>();
    auto call = client.SayHello(request, std::move(context));

    sample::ugrpc::GreetingResponse in;
    UEXPECT_NO_THROW(in = call.Finish());
    ASSERT_EQ(in.name(), "null");
}

UTEST_F(GrpcClientTestBaggage, TestGrpcClientWrongKey) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

    sample::ugrpc::GreetingRequest request;

    BaggageManager().SetBaggage("wrong_key=wrong_value");

    auto context = std::make_unique<grpc::ClientContext>();
    auto call = client.SayHello(request, std::move(context));

    sample::ugrpc::GreetingResponse in;
    UEXPECT_NO_THROW(in = call.Finish());
    ASSERT_EQ(in.name(), "");
}

USERVER_NAMESPACE_END
