#include <userver/utest/utest.hpp>

#include <tests/service_fixture_test.hpp>
#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

#include <userver/ugrpc/client/middlewares/base.hpp>

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
    return mw;
  }

  std::shared_ptr<MockMiddleware> mw = std::make_shared<MockMiddleware>();
};

}  // namespace

using GrpcMiddlewares = GrpcServiceFixture;

UTEST_F(GrpcMiddlewares, HappyPath) {
  auto mwf = std::make_shared<MockMiddlewareFactory>();
  GetMiddlewareFactories().push_back(
      std::shared_ptr<ugrpc::client::MiddlewareFactoryBase>(mwf));
  UnitTestService service;
  RegisterService(service);
  StartServer();

  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  auto& mw = mwf->mw;

  EXPECT_EQ(mw->times_called, 0);

  sample::ugrpc::GreetingRequest request;
  request.set_name("userver");
  auto response = client.SayHello(request).Finish();

  EXPECT_EQ(mw->times_called, 1);
  EXPECT_EQ(response.name(), "Hello userver");
}

UTEST_F(GrpcMiddlewares, Exception) {
  auto mwf = std::make_shared<MockMiddlewareFactory>();
  GetMiddlewareFactories().push_back(
      std::shared_ptr<ugrpc::client::MiddlewareFactoryBase>(mwf));
  UnitTestService service;
  RegisterService(service);
  StartServer();

  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  auto& mw = mwf->mw;

  EXPECT_EQ(mw->times_called, 0);

  mw->throw_exception = true;
  sample::ugrpc::GreetingRequest request;
  request.set_name("userver");
  EXPECT_THROW(auto r = client.SayHello(request), std::runtime_error);
  EXPECT_EQ(mw->times_called, 1);
}

UTEST_F(GrpcMiddlewares, DISABLED_IN_DEBUG_TEST_NAME(Drop)) {
  auto mwf = std::make_shared<MockMiddlewareFactory>();
  GetMiddlewareFactories().push_back(
      std::shared_ptr<ugrpc::client::MiddlewareFactoryBase>(mwf));
  UnitTestService service;
  RegisterService(service);
  StartServer();

  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  auto& mw = mwf->mw;

  EXPECT_EQ(mw->times_called, 0);

  mw->call_next = false;
  sample::ugrpc::GreetingRequest request;
  request.set_name("userver");
  EXPECT_THROW(client.SayHello(request).Finish(), std::exception);
  EXPECT_EQ(mw->times_called, 1);
}

USERVER_NAMESPACE_END
