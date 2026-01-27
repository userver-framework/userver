#include <gtest/gtest.h>

#include <string_view>

#include <userver/baggage/baggage_manager.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/impl/userver_experiments.hpp>

#include <ugrpc/impl/rpc_metadata.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/impl/completion_queue_pool.hpp>
#include <userver/ugrpc/client/middlewares/baggage/middleware.hpp>
#include <userver/ugrpc/impl/to_string.hpp>
#include <userver/ugrpc/server/middlewares/baggage/middleware.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

#include <dynamic_config/variables/BAGGAGE_SETTINGS.hpp>
#include <dynamic_config/variables/USERVER_BAGGAGE_ENABLED.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class ServerBaggageTestService final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext& /*context*/, sample::ugrpc::GreetingRequest&& /*request*/) override {
        sample::ugrpc::GreetingResponse response;
        const auto* bg = baggage::BaggageManager::TryGetBaggage();

        if (bg) {
            response.set_name(bg->ToString());
        } else {
            response.set_name("null");
        }

        return response;
    }
};

class GrpcServerTestBaggage
    : public ugrpc::tests::ServiceWithClientFixture<ServerBaggageTestService, sample::ugrpc::UnitTestServiceClient> {
public:
    GrpcServerTestBaggage()
        : ugrpc::tests::ServiceWithClientFixture<ServerBaggageTestService, sample::ugrpc::UnitTestServiceClient>(
              ugrpc::server::ServerConfig{},
              ugrpc::server::Middlewares{std::make_shared<ugrpc::server::middlewares::baggage::Middleware>()},
              ugrpc::client::Middlewares{}
          )
    {
        ExtendDynamicConfig({
            {::dynamic_config::BAGGAGE_SETTINGS, {{"key1", "key2", "key3"}}},
            {::dynamic_config::USERVER_BAGGAGE_ENABLED, true},
        });
    }
};

}  // namespace

UTEST_F(GrpcServerTestBaggage, TestGrpcBaggage) {
    sample::ugrpc::GreetingRequest out;
    ugrpc::client::CallOptions call_options;
    const std::string baggage = "key1=value1";

    call_options.AddMetadata(ugrpc::impl::kXBaggage, baggage);

    sample::ugrpc::GreetingResponse in;
    UEXPECT_NO_THROW(in = GetClient().SayHello(out, std::move(call_options)));
    ASSERT_EQ(in.name(), baggage);
}

UTEST_F(GrpcServerTestBaggage, TestGrpcBaggageMultiply) {
    sample::ugrpc::GreetingRequest out;
    ugrpc::client::CallOptions call_options;

    const std::string baggage = "key1=value1;key2=value2;key3";
    call_options.AddMetadata(ugrpc::impl::kXBaggage, baggage);

    sample::ugrpc::GreetingResponse in;
    UEXPECT_NO_THROW(in = GetClient().SayHello(out, std::move(call_options)));
    ASSERT_EQ(in.name(), baggage);
}

UTEST_F(GrpcServerTestBaggage, TestGrpcBaggageNoBaggage) {
    sample::ugrpc::GreetingRequest out;
    ugrpc::client::CallOptions call_options;

    sample::ugrpc::GreetingResponse in;
    UEXPECT_NO_THROW(in = GetClient().SayHello(out, std::move(call_options)));
    ASSERT_EQ(in.name(), "null");
}

UTEST_F(GrpcServerTestBaggage, TestGrpcBaggageWrongKey) {
    sample::ugrpc::GreetingRequest out;
    ugrpc::client::CallOptions call_options;

    call_options.AddMetadata(ugrpc::impl::kXBaggage, "wrong_key=wrong_value");

    sample::ugrpc::GreetingResponse in;
    UEXPECT_NO_THROW(in = GetClient().SayHello(out, std::move(call_options)));
    ASSERT_EQ(in.name(), "");
}

namespace {

class ClientBaggageTestService final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext& context, sample::ugrpc::GreetingRequest&& /*request*/) override {
        sample::ugrpc::GreetingResponse response;
        const auto*
            baggage_header = utils::FindOrNullptr(context.GetServerContext().client_metadata(), ugrpc::impl::kXBaggage);

        if (baggage_header) {
            response.set_name(ugrpc::impl::ToString(*baggage_header));
        } else {
            response.set_name("null");
        }

        return response;
    }
};

class GrpcClientTestBaggage
    : public ugrpc::tests::ServiceWithClientFixture<ClientBaggageTestService, sample::ugrpc::UnitTestServiceClient> {
public:
    GrpcClientTestBaggage()
        : ugrpc::tests::ServiceWithClientFixture<ClientBaggageTestService, sample::ugrpc::UnitTestServiceClient>(
              ugrpc::server::ServerConfig{},
              ugrpc::server::Middlewares{},
              ugrpc::client::Middlewares{std::make_shared<ugrpc::client::middlewares::baggage::Middleware>()}
          )
    {
        ExtendDynamicConfig({
            {::dynamic_config::BAGGAGE_SETTINGS, {{"key1", "key2", "key3"}}},
            {::dynamic_config::USERVER_BAGGAGE_ENABLED, true},
        });
    }

    baggage::BaggageManager& BaggageManager() { return baggage_manager_; }

private:
    baggage::BaggageManager baggage_manager_ = baggage::BaggageManager(GetConfigSource());
};

}  // namespace

UTEST_F(GrpcClientTestBaggage, TestGrpcClientBaggage) {
    const std::string baggage = "key1=value1";

    sample::ugrpc::GreetingRequest request;

    BaggageManager().SetBaggage(baggage);

    ugrpc::client::CallOptions call_options;

    sample::ugrpc::GreetingResponse in;
    UEXPECT_NO_THROW(in = GetClient().SayHello(request, std::move(call_options)));
    ASSERT_EQ(in.name(), baggage);
}

UTEST_F(GrpcClientTestBaggage, TestGrpcClientBaggageMultiply) {
    const std::string baggage = "key1=value1;key2=value2;key3";

    sample::ugrpc::GreetingRequest request;

    BaggageManager().SetBaggage(baggage);

    ugrpc::client::CallOptions call_options;

    sample::ugrpc::GreetingResponse in;
    UEXPECT_NO_THROW(in = GetClient().SayHello(request, std::move(call_options)));
    ASSERT_EQ(in.name(), baggage);
}

UTEST_F(GrpcClientTestBaggage, TestGrpcClientNoBaggage) {
    sample::ugrpc::GreetingRequest request;

    ugrpc::client::CallOptions call_options;

    sample::ugrpc::GreetingResponse in;
    UEXPECT_NO_THROW(in = GetClient().SayHello(request, std::move(call_options)));
    ASSERT_EQ(in.name(), "null");
}

UTEST_F(GrpcClientTestBaggage, TestGrpcClientWrongKey) {
    sample::ugrpc::GreetingRequest request;

    BaggageManager().SetBaggage("wrong_key=wrong_value");

    ugrpc::client::CallOptions call_options;

    sample::ugrpc::GreetingResponse in;
    UEXPECT_NO_THROW(in = GetClient().SayHello(request, std::move(call_options)));
    ASSERT_EQ(in.name(), "");
}

USERVER_NAMESPACE_END
