#include <userver/utest/utest.hpp>

#include <userver/clients/grpc/exceptions.hpp>

#include <tests/grpc_service_fixture_test.hpp>
#include "unit_test_client.usrv.pb.hpp"

USERVER_NAMESPACE_BEGIN

using namespace ::grpc::test;

class UnitTestBadServiceImpl : public UnitTestService::Service {
 public:
  ::grpc::Status SayHello(::grpc::ServerContext* /*context*/,
                          const GreetingRequest* /*request*/,
                          GreetingResponse* /*response*/) override {
    return {::grpc::StatusCode::INTERNAL, "message", "details"};
  }

  ::grpc::Status ReadMany(
      ::grpc::ServerContext* /*context*/,
      const StreamGreetingRequest* /*request*/,
      ::grpc::ServerWriter<StreamGreetingResponse>* /*writer*/) override {
    return {::grpc::StatusCode::INTERNAL, "message", "details"};
  }

  ::grpc::Status WriteMany(
      ::grpc::ServerContext* /*context*/,
      ::grpc::ServerReader<StreamGreetingRequest>* /*reader*/,
      StreamGreetingResponse* /*response*/) override {
    return {::grpc::StatusCode::INTERNAL, "message", "details"};
  }

  ::grpc::Status Chat(
      ::grpc::ServerContext* /*context*/,
      ::grpc::ServerReaderWriter<StreamGreetingResponse,
                                 StreamGreetingRequest>* /*stream*/) override {
    return {::grpc::StatusCode::INTERNAL, "message", "details"};
  }
};

using GrpcClientErrorTest = GrpcServiceFixture<UnitTestBadServiceImpl>;

UTEST_F(GrpcClientErrorTest, UnaryRPC) {
  UnitTestServiceClient client{GetChannel(), GetQueue()};
  GreetingRequest out;
  out.set_name("userver");
  EXPECT_THROW(client.SayHello(out).Finish(), clients::grpc::InternalError);
}

UTEST_F(GrpcClientErrorTest, InputStream) {
  UnitTestServiceClient client{GetChannel(), GetQueue()};
  StreamGreetingRequest out;
  out.set_name("userver");
  out.set_number(42);
  StreamGreetingResponse in;
  auto is = client.ReadMany(out);
  EXPECT_THROW((void)is.Read(in), clients::grpc::InternalError);
}

UTEST_F(GrpcClientErrorTest, OutputStream) {
  UnitTestServiceClient client{GetChannel(), GetQueue()};
  auto os = client.WriteMany();
  EXPECT_THROW(os.Finish(), clients::grpc::InternalError);
}

UTEST_F(GrpcClientErrorTest, BidirectionalStream) {
  UnitTestServiceClient client{GetChannel(), GetQueue()};
  StreamGreetingResponse in;
  auto bs = client.Chat();
  EXPECT_THROW((void)bs.Read(in), clients::grpc::InternalError);
}

USERVER_NAMESPACE_END
