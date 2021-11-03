#include <userver/utest/utest.hpp>

#include <userver/utils/algo.hpp>

#include <tests/grpc_service_fixture_test.hpp>
#include "unit_test_client.usrv.pb.hpp"

USERVER_NAMESPACE_BEGIN

using namespace ::grpc::test;

void CheckServerContext(::grpc::ServerContext& context) {
  const auto& client_metadata = context.client_metadata();
  EXPECT_EQ(utils::FindOptional(client_metadata, "req_header"), "value");
  context.AddTrailingMetadata("resp_header", "value");
}

class UnitTestServiceImpl : public UnitTestService::Service {
 public:
  ::grpc::Status SayHello(::grpc::ServerContext* context,
                          const GreetingRequest* request,
                          GreetingResponse* response) override {
    if (request->name() != "default_context") {
      CheckServerContext(*context);
    }
    response->set_name("Hello " + request->name());
    return ::grpc::Status::OK;
  }

  ::grpc::Status ReadMany(
      ::grpc::ServerContext* context, const StreamGreetingRequest* request,
      ::grpc::ServerWriter<StreamGreetingResponse>* writer) override {
    CheckServerContext(*context);
    StreamGreetingResponse response;
    response.set_name("Hello again " + request->name());
    for (auto i = 0; i < request->number(); ++i) {
      response.set_number(i);
      writer->Write(response);
    }
    return ::grpc::Status::OK;
  }

  ::grpc::Status WriteMany(::grpc::ServerContext* context,
                           ::grpc::ServerReader<StreamGreetingRequest>* reader,
                           StreamGreetingResponse* response) override {
    CheckServerContext(*context);
    StreamGreetingRequest request;
    int count = 0;
    while (reader->Read(&request)) {
      ++count;
    }
    response->set_name("Hello");
    response->set_number(count);
    return ::grpc::Status::OK;
  }

  ::grpc::Status Chat(
      ::grpc::ServerContext* context,
      ::grpc::ServerReaderWriter<StreamGreetingResponse, StreamGreetingRequest>*
          stream) override {
    CheckServerContext(*context);
    StreamGreetingRequest request;
    StreamGreetingResponse response;
    int count = 0;
    while (stream->Read(&request)) {
      ++count;
      response.set_number(count);
      response.set_name("Hello " + request.name());
      stream->Write(response);
    }
    return ::grpc::Status::OK;
  }
};

using GrpcClientTest = GrpcServiceFixture<UnitTestServiceImpl>;

std::unique_ptr<::grpc::ClientContext> PrepareClientContext() {
  auto context = std::make_unique<::grpc::ClientContext>();
  context->AddMetadata("req_header", "value");
  return context;
}

void CheckClientContext(const ::grpc::ClientContext& context) {
  const auto& metadata = context.GetServerTrailingMetadata();
  const auto iter = metadata.find("resp_header");
  ASSERT_NE(iter, metadata.end());
  EXPECT_EQ(iter->second, "value");
}

UTEST_F(GrpcClientTest, UnaryRPC) {
  UnitTestServiceClient client{GetChannel(), GetQueue()};
  GreetingRequest out;
  out.set_name("userver");
  auto call = client.SayHello(out, PrepareClientContext());

  GreetingResponse in;
  EXPECT_NO_THROW(in = call.Finish());
  CheckClientContext(call.GetContext());
  EXPECT_EQ("Hello " + out.name(), in.name());
}

UTEST_F(GrpcClientTest, UnaryRPCDefaultContext) {
  UnitTestServiceClient client{GetChannel(), GetQueue()};
  GreetingRequest out;
  out.set_name("default_context");

  GreetingResponse in;
  EXPECT_NO_THROW(in = client.SayHello(out).Finish());
  EXPECT_EQ("Hello " + out.name(), in.name());
}

UTEST_F(GrpcClientTest, InputStream) {
  constexpr int kNumber = 42;

  UnitTestServiceClient client{GetChannel(), GetQueue()};
  StreamGreetingRequest out;
  out.set_name("userver");
  out.set_number(kNumber);
  auto is = client.ReadMany(out, PrepareClientContext());

  StreamGreetingResponse in;
  for (auto i = 0; i < kNumber; ++i) {
    EXPECT_TRUE(is.Read(in));
    EXPECT_EQ(in.number(), i);
  }
  EXPECT_FALSE(is.Read(in));

  CheckClientContext(is.GetContext());
}

UTEST_F(GrpcClientTest, EmptyInputStream) {
  UnitTestServiceClient client{GetChannel(), GetQueue()};
  StreamGreetingRequest out;
  out.set_name("userver");
  out.set_number(0);
  auto is = client.ReadMany(out, PrepareClientContext());

  StreamGreetingResponse in;
  EXPECT_FALSE(is.Read(in));
  CheckClientContext(is.GetContext());
}

UTEST_F(GrpcClientTest, OutputStream) {
  UnitTestServiceClient client{GetChannel(), GetQueue()};
  auto number = 42;
  auto os = client.WriteMany(PrepareClientContext());

  StreamGreetingRequest out;
  out.set_name("userver");
  for (auto i = 0; i < number; ++i) {
    out.set_number(i);
    EXPECT_NO_THROW(os.Write(out));
  }

  StreamGreetingResponse in;
  EXPECT_NO_THROW(in = os.Finish());
  EXPECT_EQ(in.number(), number);
  CheckClientContext(os.GetContext());
}

UTEST_F(GrpcClientTest, EmptyOutputStream) {
  UnitTestServiceClient client{GetChannel(), GetQueue()};
  auto os = client.WriteMany(PrepareClientContext());

  StreamGreetingResponse in;
  EXPECT_NO_THROW(in = os.Finish());
  EXPECT_EQ(in.number(), 0);
  CheckClientContext(os.GetContext());
}

UTEST_F(GrpcClientTest, BidirectionalStream) {
  UnitTestServiceClient client{GetChannel(), GetQueue()};
  auto bs = client.Chat(PrepareClientContext());

  StreamGreetingRequest out;
  out.set_name("userver");
  StreamGreetingResponse in;

  for (auto i = 0; i < 42; ++i) {
    out.set_number(i);
    EXPECT_NO_THROW(bs.Write(out));
    EXPECT_TRUE(bs.Read(in));
    EXPECT_EQ(in.number(), i + 1);
  }
  EXPECT_NO_THROW(bs.WritesDone());
  EXPECT_FALSE(bs.Read(in));
  CheckClientContext(bs.GetContext());
}

UTEST_F(GrpcClientTest, EmptyBidirectionalStream) {
  UnitTestServiceClient client{GetChannel(), GetQueue()};
  auto bs = client.Chat(PrepareClientContext());

  StreamGreetingResponse in;
  EXPECT_NO_THROW(bs.WritesDone());
  EXPECT_FALSE(bs.Read(in));
  CheckClientContext(bs.GetContext());
}

USERVER_NAMESPACE_END
