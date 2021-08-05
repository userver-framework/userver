#include <userver/utest/utest.hpp>

#include "unit_test.usrv.pb.hpp"

#include <tests/grpc_service_fixture_test.hpp>
#include <userver/clients/grpc/errors.hpp>
#include <userver/clients/grpc/service.hpp>

namespace clients::grpc::test {

class UnitTestBadServiceImpl : public UnitTestService::Service {
 public:
  ::grpc::Status SayHello(::grpc::ServerContext* /*context*/,
                          const Greeting* /*request*/,
                          Greeting* /*response*/) override {
    return {::grpc::StatusCode::INTERNAL, "message", "details"};
  }

  ::grpc::Status ReadMany(
      ::grpc::ServerContext* /*context*/, const StreamGreeting* /*request*/,
      ::grpc::ServerWriter<StreamGreeting>* /*writer*/) override {
    return {::grpc::StatusCode::INTERNAL, "message", "details"};
  }

  ::grpc::Status WriteMany(::grpc::ServerContext* /*context*/,
                           ::grpc::ServerReader<StreamGreeting>* /*reader*/,
                           StreamGreeting* /*response*/) override {
    return {::grpc::StatusCode::INTERNAL, "message", "details"};
  }

  ::grpc::Status Chat(
      ::grpc::ServerContext* /*context*/,
      ::grpc::ServerReaderWriter<StreamGreeting, StreamGreeting>* /*stream*/)
      override {
    return {::grpc::StatusCode::INTERNAL, "message", "details"};
  }
};

using GrpcClientErrorTest =
    GrpcServiceFixture<UnitTestService, UnitTestBadServiceImpl>;

UTEST_F(GrpcClientErrorTest, SimpleRPC) {
  UnitTestServiceClient client{ClientChannel(), GetQueue()};
  Greeting out;
  out.set_name("userver");
  EXPECT_THROW(client.SayHello(out), InternalError);
}

UTEST_F(GrpcClientErrorTest, ServerClientStream) {
  UnitTestServiceClient client{ClientChannel(), GetQueue()};
  auto number = 42;
  StreamGreeting out;
  out.set_name("userver");
  out.set_number(number);
  StreamGreeting in;
  in.set_number(number);
  auto is = client.ReadMany(out);
  EXPECT_THROW(is >> in, BaseError);
}

UTEST_F(GrpcClientErrorTest, ClientServerStream) {
  UnitTestServiceClient client{ClientChannel(), GetQueue()};
  auto os = client.WriteMany();
  EXPECT_THROW(os.GetResponse(), BaseError);
}

UTEST_F(GrpcClientErrorTest, BidirStream) {
  UnitTestServiceClient client{ClientChannel(), GetQueue()};
  StreamGreeting in;
  auto bs = client.Chat();
  EXPECT_THROW(bs >> in, BaseError);
}

}  // namespace clients::grpc::test
