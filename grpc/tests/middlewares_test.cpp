#include <userver/utest/utest.hpp>

#include <userver/ugrpc/client/middlewares/base.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class UnitTestService final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext& /*context*/, sample::ugrpc::GreetingRequest&& request) override {
        sample::ugrpc::GreetingResponse response;
        response.set_name("Hello " + request.name());
        return response;
    }

    ReadManyResult ReadMany(
        CallContext& /*context*/,
        sample::ugrpc::StreamGreetingRequest&& request,
        ReadManyWriter& writer
    ) override {
        sample::ugrpc::StreamGreetingResponse response;
        response.set_name("Hello again " + request.name());
        for (int i = 0; i < request.number(); ++i) {
            response.set_number(i);
            writer.Write(response);
        }
        return grpc::Status::OK;
    }

    WriteManyResult WriteMany(CallContext& /*context*/, WriteManyReader& reader) override {
        sample::ugrpc::StreamGreetingRequest request;
        int count = 0;
        while (reader.Read(request)) {
            ++count;
        }
        sample::ugrpc::StreamGreetingResponse response;
        response.set_name("Hello");
        response.set_number(count);
        return response;
    }

    ChatResult Chat(CallContext& /*context*/, ChatReaderWriter& stream) override {
        sample::ugrpc::StreamGreetingRequest request;
        sample::ugrpc::StreamGreetingResponse response;
        int count = 0;
        while (stream.Read(request)) {
            ++count;
            response.set_number(count);
            response.set_name("Hello " + request.name());
            stream.Write(response);
        }
        return grpc::Status::OK;
    }
};

class MockMiddleware final : public ugrpc::client::MiddlewareBase {
public:
    void PreStartCall(ugrpc::client::MiddlewareCallContext& /*context*/) const override {
        times_called++;
        if (throw_exception) throw std::runtime_error("error");
    }

    mutable std::size_t times_called{0};
    bool throw_exception{false};
};

class MockMiddlewareFactory final : public ugrpc::client::MiddlewareFactoryBase {
public:
    std::shared_ptr<const ugrpc::client::MiddlewareBase> GetMiddleware(std::string_view) const override { return mw_; }

    MockMiddleware& GetMockMiddleware() { return *mw_; }

private:
    std::shared_ptr<MockMiddleware> mw_ = std::make_shared<MockMiddleware>();
};

class GrpcMiddlewares : public ugrpc::tests::ServiceFixtureBase {
protected:
    GrpcMiddlewares() {
        SetClientMiddlewareFactories({mwf_});
        RegisterService(service_);
        StartServer();
        client_.emplace(MakeClient<sample::ugrpc::UnitTestServiceClient>());
    }

    ~GrpcMiddlewares() override {
        client_.reset();
        StopServer();
    }

    sample::ugrpc::UnitTestServiceClient& GetClient() { return client_.value(); }

    MockMiddleware& GetMockMiddleware() { return mwf_->GetMockMiddleware(); }

private:
    std::shared_ptr<MockMiddlewareFactory> mwf_ = std::make_shared<MockMiddlewareFactory>();
    UnitTestService service_;
    std::optional<sample::ugrpc::UnitTestServiceClient> client_;
};

using GrpcMiddlewaresDeathTest = GrpcMiddlewares;

}  // namespace

UTEST_F(GrpcMiddlewares, HappyPath) {
    EXPECT_EQ(GetMockMiddleware().times_called, 0);

    sample::ugrpc::GreetingRequest request;
    request.set_name("userver");
    auto response = GetClient().SyncSayHello(request);

    EXPECT_EQ(GetMockMiddleware().times_called, 1);
    EXPECT_EQ(response.name(), "Hello userver");
}

UTEST_F(GrpcMiddlewares, Exception) {
    EXPECT_EQ(GetMockMiddleware().times_called, 0);

    GetMockMiddleware().throw_exception = true;
    sample::ugrpc::GreetingRequest request;
    request.set_name("userver");
    UEXPECT_THROW(auto future = GetClient().AsyncSayHello(request), std::runtime_error);
    EXPECT_EQ(GetMockMiddleware().times_called, 1);
}

USERVER_NAMESPACE_END
