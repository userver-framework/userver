#include <userver/utest/utest.hpp>

#include <userver/ugrpc/client/middlewares/base.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class UnitTestService final : public sample::ugrpc::UnitTestServiceBase {
 public:
  void SayHello(SayHelloCall& call,
                sample::ugrpc::GreetingRequest&& request) override {
    sample::ugrpc::GreetingResponse response;
    response.set_name("Hello " + request.name());
    call.Finish(response);
  }

  void ReadMany(ReadManyCall& call,
                sample::ugrpc::StreamGreetingRequest&& request) override {
    sample::ugrpc::StreamGreetingResponse response;
    response.set_name("Hello again " + request.name());
    for (int i = 0; i < request.number(); ++i) {
      response.set_number(i);
      call.Write(response);
    }
    call.Finish();
  }

  void WriteMany(WriteManyCall& call) override {
    sample::ugrpc::StreamGreetingRequest request;
    int count = 0;
    while (call.Read(request)) {
      ++count;
    }
    sample::ugrpc::StreamGreetingResponse response;
    response.set_name("Hello");
    response.set_number(count);
    call.Finish(response);
  }

  void Chat(ChatCall& call) override {
    sample::ugrpc::StreamGreetingRequest request;
    sample::ugrpc::StreamGreetingResponse response;
    int count = 0;
    while (call.Read(request)) {
      ++count;
      response.set_number(count);
      response.set_name("Hello " + request.name());
      call.Write(response);
    }
    call.Finish();
  }
};

class MockMiddleware final : public ugrpc::client::MiddlewareBase {
 public:
  void Handle(ugrpc::client::MiddlewareCallContext& context) const override {
    times_called++;
    if (throw_exception) throw std::runtime_error("error");
    if (call_next) context.Next();
  }

  mutable std::size_t times_called{0};
  bool call_next{true};
  bool throw_exception{false};
};

class MockMiddlewareFactory final
    : public ugrpc::client::MiddlewareFactoryBase {
 public:
  std::shared_ptr<const ugrpc::client::MiddlewareBase> GetMiddleware(
      std::string_view) const override {
    return mw_;
  }

  MockMiddleware& GetMockMiddleware() { return *mw_; }

 private:
  std::shared_ptr<MockMiddleware> mw_ = std::make_shared<MockMiddleware>();
};

class GrpcMiddlewares : public ugrpc::tests::ServiceFixtureBase {
 protected:
  GrpcMiddlewares() {
    AddClientMiddleware(mwf_);
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
  std::shared_ptr<MockMiddlewareFactory> mwf_ =
      std::make_shared<MockMiddlewareFactory>();
  UnitTestService service_;
  std::optional<sample::ugrpc::UnitTestServiceClient> client_;
};

using GrpcMiddlewaresDeathTest = GrpcMiddlewares;

}  // namespace

UTEST_F(GrpcMiddlewares, HappyPath) {
  EXPECT_EQ(GetMockMiddleware().times_called, 0);

  sample::ugrpc::GreetingRequest request;
  request.set_name("userver");
  auto response = GetClient().SayHello(request).Finish();

  EXPECT_EQ(GetMockMiddleware().times_called, 1);
  EXPECT_EQ(response.name(), "Hello userver");
}

UTEST_F(GrpcMiddlewares, Exception) {
  EXPECT_EQ(GetMockMiddleware().times_called, 0);

  GetMockMiddleware().throw_exception = true;
  sample::ugrpc::GreetingRequest request;
  request.set_name("userver");
  UEXPECT_THROW(auto r = GetClient().SayHello(request), std::runtime_error);
  EXPECT_EQ(GetMockMiddleware().times_called, 1);
}

UTEST_F_DEATH(GrpcMiddlewaresDeathTest, Drop) {
  EXPECT_EQ(GetMockMiddleware().times_called, 0);

  GetMockMiddleware().call_next = false;
  sample::ugrpc::GreetingRequest request;
  request.set_name("userver");
  EXPECT_UINVARIANT_FAILURE_MSG(GetClient().SayHello(request).Finish(),
                                "forgot to call context.Next()");
}

USERVER_NAMESPACE_END
