#include <userver/utest/utest.hpp>

#include "unit_test.usrv.pb.hpp"

#include <tests/grpc_service_fixture_test.hpp>
#include <userver/clients/grpc/errors.hpp>
#include <userver/clients/grpc/service.hpp>

namespace clients::grpc::test {

void CheckServerContext(::grpc::ServerContext* context) {
  const auto& metadata = context->client_metadata();
  EXPECT_EQ(metadata.find("req_header")->second, "value");
  context->AddTrailingMetadata("resp_header", "value");
}

class UnitTestServiceImpl : public UnitTestService::Service {
 public:
  ::grpc::Status SayHello(::grpc::ServerContext* context,
                          const Greeting* request,
                          Greeting* response) override {
    if (request->name() != "default_context") {
      CheckServerContext(context);
    }
    static const std::string prefix("Hello ");
    response->set_name(prefix + request->name());
    return ::grpc::Status::OK;
  }

  ::grpc::Status ReadMany(
      ::grpc::ServerContext* context, const StreamGreeting* request,
      ::grpc::ServerWriter<StreamGreeting>* writer) override {
    CheckServerContext(context);
    static const std::string prefix("Hello again ");

    StreamGreeting sg;
    sg.set_name(prefix + request->name());
    for (auto i = 0; i < request->number(); ++i) {
      sg.set_number(i);
      writer->Write(sg);
    }
    return ::grpc::Status::OK;
  }

  ::grpc::Status WriteMany(::grpc::ServerContext* context,
                           ::grpc::ServerReader<StreamGreeting>* reader,
                           StreamGreeting* response) override {
    CheckServerContext(context);
    StreamGreeting in;
    int count = 0;
    while (reader->Read(&in)) {
      ++count;
    }
    response->set_name("Hello blabber");
    response->set_number(count);
    return ::grpc::Status::OK;
  }

  ::grpc::Status Chat(
      ::grpc::ServerContext* context,
      ::grpc::ServerReaderWriter<StreamGreeting, StreamGreeting>* stream)
      override {
    CheckServerContext(context);
    static const std::string prefix("Hello ");
    StreamGreeting in;
    StreamGreeting out;
    int count = 0;
    while (stream->Read(&in)) {
      ++count;
      out.set_number(count);
      out.set_name(prefix + in.name());
      stream->Write(out);
    }

    return ::grpc::Status::OK;
  }
};

using GrpcClientTest = GrpcServiceFixture<UnitTestService, UnitTestServiceImpl>;

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
  // The test was flaky, running multiple iterations to simplify error
  // detection.
  for (int i = 0; i < 100; ++i) {
    UnitTestServiceClient client{ClientChannel(), GetQueue()};
    Greeting out;
    out.set_name("userver");
    auto call = client.SayHello(out, PrepareClientContext());

    Greeting in;
    EXPECT_NO_THROW(in = call.Finish());
    CheckClientContext(call.GetContext());
    EXPECT_EQ("Hello " + out.name(), in.name());
  }
}

UTEST_F(GrpcClientTest, UnaryRPCDefaultContext) {
  UnitTestServiceClient client{ClientChannel(), GetQueue()};
  Greeting out;
  out.set_name("default_context");

  Greeting in;
  EXPECT_NO_THROW(in = client.SayHello(out).Finish());
  EXPECT_EQ("Hello " + out.name(), in.name());
}

UTEST_F(GrpcClientTest, InputStream) {
  // The test was flaky, running multiple iterations to simplify error
  // detection.
  for (int j = 0; j < 100; ++j) {
    UnitTestServiceClient client{ClientChannel(), GetQueue()};
    auto number = 42;
    StreamGreeting out;
    out.set_name("userver");
    out.set_number(number);
    auto is = client.ReadMany(out, PrepareClientContext());

    StreamGreeting in;
    for (auto i = 0; i < number; ++i) {
      EXPECT_TRUE(is.Read(in));
      EXPECT_EQ(in.number(), i);
    }
    EXPECT_FALSE(is.Read(in));

    CheckClientContext(is.GetContext());
  }
}

UTEST_F(GrpcClientTest, EmptyInputStream) {
  // The test was flaky, running multiple iterations to simplify error
  // detection.
  for (int i = 0; i < 100; ++i) {
    UnitTestServiceClient client{ClientChannel(), GetQueue()};
    auto context = PrepareClientContext();
    StreamGreeting out;
    out.set_name("userver");
    out.set_number(0);
    auto is = client.ReadMany(out, std::move(context));

    StreamGreeting in;
    EXPECT_FALSE(is.Read(in));
    CheckClientContext(is.GetContext());
  }
}

UTEST_F(GrpcClientTest, OutputStream) {
  UnitTestServiceClient client{ClientChannel(), GetQueue()};
  auto number = 42;
  auto os = client.WriteMany(PrepareClientContext());

  StreamGreeting out;
  out.set_name("userver");
  for (auto i = 0; i < number; ++i) {
    out.set_number(i);
    EXPECT_NO_THROW(os.Write(out));
  }

  StreamGreeting in;
  EXPECT_NO_THROW(in = os.Finish());
  EXPECT_EQ(in.number(), number);
  CheckClientContext(os.GetContext());
}

UTEST_F(GrpcClientTest, EmptyOutputStream) {
  UnitTestServiceClient client{ClientChannel(), GetQueue()};
  auto os = client.WriteMany(PrepareClientContext());

  StreamGreeting in;
  EXPECT_NO_THROW(in = os.Finish());
  EXPECT_EQ(in.number(), 0);
  CheckClientContext(os.GetContext());
}

UTEST_F(GrpcClientTest, BidirectionalStream) {
  UnitTestServiceClient client{ClientChannel(), GetQueue()};
  auto bs = client.Chat(PrepareClientContext());

  StreamGreeting out;
  out.set_name("userver");
  StreamGreeting in;

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
  UnitTestServiceClient client{ClientChannel(), GetQueue()};
  auto bs = client.Chat(PrepareClientContext());

  StreamGreeting in;
  EXPECT_NO_THROW(bs.WritesDone());
  EXPECT_FALSE(bs.Read(in));
  CheckClientContext(bs.GetContext());
}

}  // namespace clients::grpc::test
