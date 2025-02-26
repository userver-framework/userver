#include <userver/utest/utest.hpp>

#include <userver/engine/sleep.hpp>

#include <userver/ugrpc/client/middlewares/base.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class UnitTestService final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext& /*context*/, sample::ugrpc::GreetingRequest&& request) override {
        SleepIfNeeded();
        sample::ugrpc::GreetingResponse response;
        response.set_name("Hello " + request.name());
        return response;
    }

    ReadManyResult ReadMany(
        CallContext& /*context*/,
        sample::ugrpc::StreamGreetingRequest&& request,
        ReadManyWriter& writer
    ) override {
        SleepIfNeeded();
        sample::ugrpc::StreamGreetingResponse response;
        response.set_name("Hello again " + request.name());
        for (int i = 0; i < request.number(); ++i) {
            response.set_number(i);
            writer.Write(response);
        }
        return grpc::Status::OK;
    }

    WriteManyResult WriteMany(CallContext& /*context*/, WriteManyReader& reader) override {
        SleepIfNeeded();
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
        SleepIfNeeded();
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

    void SetResponsesBlocked(bool blocked) { responses_blocked_ = blocked; }

private:
    void SleepIfNeeded() {
        if (responses_blocked_) {
            engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
        }
    }

    bool responses_blocked_{false};
};

class MockMiddleware final : public ugrpc::client::MiddlewareBase {
public:
    void PreStartCall(ugrpc::client::MiddlewareCallContext& /*context*/) const override {
        ++times_called;
        if (throw_exception) throw std::runtime_error("error");
    }

    mutable std::size_t times_called{0};
    bool throw_exception{false};
};

template <typename Middleware>
class SimpleMiddlewareFactory final : public ugrpc::client::MiddlewareFactoryBase {
public:
    template <typename... Args>
    explicit SimpleMiddlewareFactory(std::in_place_t, Args&&... args)
        : mw_(std::make_shared<Middleware>(std::forward<Args>(args)...)) {}

    std::shared_ptr<const ugrpc::client::MiddlewareBase> GetMiddleware(std::string_view) const override { return mw_; }

    Middleware& GetTypedMiddleware() { return *mw_; }

private:
    std::shared_ptr<Middleware> mw_;
};

template <typename Middleware>
class MiddlewareFixture : public ugrpc::tests::ServiceFixtureBase {
protected:
    MiddlewareFixture() {
        SetClientMiddlewareFactories({mwf_});
        RegisterService(service_);
        StartServer();
        client_.emplace(MakeClient<sample::ugrpc::UnitTestServiceClient>());
    }

    ~MiddlewareFixture() override {
        client_.reset();
        StopServer();
    }

    sample::ugrpc::UnitTestServiceClient& GetClient() { return client_.value(); }

    UnitTestService& GetService() { return service_; }

    Middleware& GetMiddleware() { return mwf_->GetTypedMiddleware(); }

private:
    std::shared_ptr<SimpleMiddlewareFactory<Middleware>> mwf_ =
        std::make_shared<SimpleMiddlewareFactory<Middleware>>(std::in_place);
    UnitTestService service_;
    std::optional<sample::ugrpc::UnitTestServiceClient> client_;
};

using GrpcMiddlewares = MiddlewareFixture<MockMiddleware>;

}  // namespace

UTEST_F(GrpcMiddlewares, HappyPath) {
    EXPECT_EQ(GetMiddleware().times_called, 0);

    sample::ugrpc::GreetingRequest request;
    request.set_name("userver");
    auto response = GetClient().SayHello(request);

    EXPECT_EQ(GetMiddleware().times_called, 1);
    EXPECT_EQ(response.name(), "Hello userver");
}

UTEST_F(GrpcMiddlewares, Exception) {
    EXPECT_EQ(GetMiddleware().times_called, 0);

    GetMiddleware().throw_exception = true;
    sample::ugrpc::GreetingRequest request;
    request.set_name("userver");
    UEXPECT_THROW(auto future = GetClient().AsyncSayHello(request), std::runtime_error);
    EXPECT_EQ(GetMiddleware().times_called, 1);
}

namespace {

class ThrowingOnFinishMiddleware final : public ugrpc::client::MiddlewareBase {
public:
    void PostFinish(ugrpc::client::MiddlewareCallContext& /*context*/, const grpc::Status& /*status*/) const override {
        ++times_called;
        if (throw_exception) throw std::runtime_error("mock error");
    }

    mutable std::size_t times_called{0};
    bool throw_exception{false};
};

using GrpcPostFinishMiddleware = MiddlewareFixture<ThrowingOnFinishMiddleware>;

}  // namespace

UTEST_F(GrpcPostFinishMiddleware, HappyPath) {
    EXPECT_EQ(GetMiddleware().times_called, 0);

    sample::ugrpc::GreetingRequest request;
    request.set_name("userver");
    auto future = GetClient().AsyncSayHello(request);

    engine::SleepFor(std::chrono::milliseconds{100});
    EXPECT_EQ(GetMiddleware().times_called, 0);

    const auto status = future.WaitUntil({});
    EXPECT_EQ(status, engine::FutureStatus::kReady);
    EXPECT_EQ(GetMiddleware().times_called, 1);

    const auto response = future.Get();
    EXPECT_EQ(response.name(), "Hello userver");
}

UTEST_F(GrpcPostFinishMiddleware, Exception) {
    EXPECT_EQ(GetMiddleware().times_called, 0);
    GetMiddleware().throw_exception = true;

    sample::ugrpc::GreetingRequest request;
    request.set_name("userver");
    auto future = GetClient().AsyncSayHello(request);

    engine::SleepFor(std::chrono::milliseconds{100});
    EXPECT_EQ(GetMiddleware().times_called, 0);

    const auto status = future.WaitUntil({});
    EXPECT_EQ(status, engine::FutureStatus::kReady);
    EXPECT_EQ(GetMiddleware().times_called, 1);

    UEXPECT_THROW_MSG(future.Get(), std::runtime_error, "mock error");
}

UTEST_F(GrpcPostFinishMiddleware, ExceptionWhenCancelled) {
    EXPECT_EQ(GetMiddleware().times_called, 0);
    GetMiddleware().throw_exception = true;
    GetService().SetResponsesBlocked(true);

    {
        sample::ugrpc::GreetingRequest request;
        request.set_name("userver");
        auto future = GetClient().AsyncSayHello(request);

        engine::current_task::GetCancellationToken().RequestCancel();

        EXPECT_EQ(GetMiddleware().times_called, 0);
        // The destructor of `future` will cancel the RPC and await grpcpp cleanup, then run middlewares.
        // The exception from PostFinish should not lead to a crash.
    }

    EXPECT_EQ(GetMiddleware().times_called, 1);
}

USERVER_NAMESPACE_END
