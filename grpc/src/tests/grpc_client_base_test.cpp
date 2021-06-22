#include <gtest/gtest.h>

#include "unit_test.usrv.pb.hpp"

#include <clients/grpc/errors.hpp>
#include <clients/grpc/service.hpp>
#include <tests/grpc_service_fixture_test.hpp>

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

std::shared_ptr<::grpc::ClientContext> PrepareClientContext() {
  auto context = std::make_shared<::grpc::ClientContext>();
  context->AddMetadata("req_header", "value");
  return context;
}

void CheckClientContext(const std::shared_ptr<::grpc::ClientContext>& context) {
  const auto& metadata = context->GetServerTrailingMetadata();
  EXPECT_EQ(metadata.find("resp_header")->second, "value");
}

TEST_F(GrpcClientTest, SimpleRPC) {
  RunTestInCoro([&] {
    UnitTestServiceClient client{ClientChannel(), GetQueue()};
    auto context = PrepareClientContext();
    Greeting out;
    out.set_name("userver");
    Greeting in;
    EXPECT_NO_THROW(in = client.SayHello(out, context));
    CheckClientContext(context);
    EXPECT_EQ("Hello " + out.name(), in.name());
  });
}

TEST_F(GrpcClientTest, SimpleRPCDefaultContext) {
  RunTestInCoro([&] {
    UnitTestServiceClient client{ClientChannel(), GetQueue()};
    Greeting out;
    out.set_name("default_context");
    Greeting in;
    EXPECT_NO_THROW(in = client.SayHello(out));
    EXPECT_EQ("Hello " + out.name(), in.name());
  });
}

// TODO TAXICOMMON-3809 fix and enable the tests
// Test is disabled because it's currently flapping
TEST_F(GrpcClientTest, DISABLED_ServerClientStream) {
  RunTestInCoro([&] {
    UnitTestServiceClient client{ClientChannel(), GetQueue()};
    auto context = PrepareClientContext();
    auto number = 42;
    StreamGreeting out;
    out.set_name("userver");
    out.set_number(number);
    StreamGreeting in;
    in.set_number(number);
    auto is = client.ReadMany(out, context);

    for (auto i = 0; i < number; ++i) {
      EXPECT_FALSE(is.IsReadFinished()) << "Read value #" << i;
      EXPECT_NO_THROW(is >> in) << "Read value #" << i;
      EXPECT_EQ(i, in.number());
    }
    EXPECT_THROW(is >> in, ValueReadError);
    // TODO TAXICOMMON-1874 reenable after fix
    // EXPECT_TRUE(is.IsReadFinished());
    CheckClientContext(context);
  });
}

// Test is disabled because it's currently flapping
TEST_F(GrpcClientTest, DISABLED_ServerClientEmptyStream) {
  RunTestInCoro([&] {
    UnitTestServiceClient client{ClientChannel(), GetQueue()};
    auto context = PrepareClientContext();
    StreamGreeting out;
    out.set_name("userver");
    out.set_number(0);
    auto is = client.ReadMany(out, context);
    StreamGreeting in;
    EXPECT_TRUE(is.IsReadFinished());
    CheckClientContext(context);
    EXPECT_THROW(is >> in, ValueReadError);
  });
}

TEST_F(GrpcClientTest, ClientServerStream) {
  RunTestInCoro([&] {
    UnitTestServiceClient client{ClientChannel(), GetQueue()};
    auto context = PrepareClientContext();
    auto number = 42;
    auto os = client.WriteMany(context);
    StreamGreeting out;
    out.set_name("userver");
    for (auto i = 0; i < number; ++i) {
      out.set_number(i);
      EXPECT_NO_THROW(os << out);
      EXPECT_TRUE(os);
    }
    StreamGreeting in;
    EXPECT_NO_THROW(in = os.GetResponse());
    CheckClientContext(context);
    EXPECT_EQ(number, in.number());
    EXPECT_THROW(os << out, StreamClosedError);
  });
}

TEST_F(GrpcClientTest, ClientServerEmptyStream) {
  RunTestInCoro([&] {
    UnitTestServiceClient client{ClientChannel(), GetQueue()};
    auto context = PrepareClientContext();
    auto os = client.WriteMany(context);
    StreamGreeting in;
    EXPECT_NO_THROW(in = os.GetResponse());
    CheckClientContext(context);
    EXPECT_EQ(0, in.number());
  });
}

TEST_F(GrpcClientTest, BidirStream) {
  RunTestInCoro([&] {
    UnitTestServiceClient client{ClientChannel(), GetQueue()};
    auto context = PrepareClientContext();
    auto number = 42;
    auto bs = client.Chat(context);

    StreamGreeting out;
    out.set_name("userver");
    StreamGreeting in;

    for (auto i = 0; i < number; ++i) {
      out.set_number(i);
      EXPECT_NO_THROW(bs << out);
      EXPECT_NO_THROW(bs >> in);
      EXPECT_EQ(i + 1, in.number());
    }
    EXPECT_NO_THROW(bs.FinishWrites());
    while (!bs.IsReadFinished()) {
    }
    CheckClientContext(context);
  });
}

TEST_F(GrpcClientTest, BidirEmptyStream) {
  RunTestInCoro([&] {
    UnitTestServiceClient client{ClientChannel(), GetQueue()};
    auto context = PrepareClientContext();
    auto bs = client.Chat(context);
    EXPECT_NO_THROW(bs.FinishWrites());
    while (!bs.IsReadFinished()) {
    }
    CheckClientContext(context);
  });
}

}  // namespace clients::grpc::test
