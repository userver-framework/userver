#include <gtest/gtest.h>
#include <thread>

#include "unit_test.usrv.pb.hpp"

#include <clients/grpc/service.hpp>
#include <tests/grpc_service_fixture_test.hpp>

#include <utest/utest.hpp>

namespace clients::grpc::test {

class UnitTestServiceImpl : public UnitTestService::Service {
 public:
  ::grpc::Status SayHello(::grpc::ServerContext* context,
                          const Greeting* request,
                          Greeting* response) override {
    static const std::string prefix("Hello ");
    response->set_name(prefix + request->name());
    return ::grpc::Status::OK;
  }

  ::grpc::Status ReadMany(
      ::grpc::ServerContext* context, const StreamGreeting* request,
      ::grpc::ServerWriter<StreamGreeting>* writer) override {
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

TEST_F(GrpcClientTest, SimpleRPC) {
  RunTestInCoro([&] {
    UnitTestServiceClient client{ClientChannel(), GetQueue()};
    Greeting out;
    out.set_name("userver");
    Greeting in;
    EXPECT_NO_THROW(in = client.SayHello(out));
    EXPECT_EQ("Hello " + out.name(), in.name());
  });
}

TEST_F(GrpcClientTest, ServerClientStream) {
  RunTestInCoro([&] {
    UnitTestServiceClient client{ClientChannel(), GetQueue()};
    auto number = 42;
    StreamGreeting out;
    out.set_name("userver");
    out.set_number(number);
    StreamGreeting in;
    in.set_number(number);
    auto is = client.ReadMany(out);

    for (auto i = 0; i < number; ++i) {
      EXPECT_FALSE(is.IsReadFinished()) << "Read value #" << i;
      EXPECT_NO_THROW(is >> in) << "Read value #" << i;
      EXPECT_EQ(i, in.number());
    }
    EXPECT_ANY_THROW(is >> in);  // TODO Specific exception
    EXPECT_TRUE(is.IsReadFinished());
  });
}

// Test is disabled because it's currently flapping
TEST_F(GrpcClientTest, DISABLED_ServerClientEmptyStream) {
  RunTestInCoro([&] {
    UnitTestServiceClient client{ClientChannel(), GetQueue()};
    StreamGreeting out;
    out.set_name("userver");
    out.set_number(0);
    auto is = client.ReadMany(out);
    StreamGreeting in;
    EXPECT_TRUE(is.IsReadFinished());
    EXPECT_ANY_THROW(is >> in);  // TODO Specific exception
  });
}

TEST_F(GrpcClientTest, ClientServerStream) {
  RunTestInCoro([&] {
    UnitTestServiceClient client{ClientChannel(), GetQueue()};
    auto number = 42;
    auto os = client.WriteMany();
    StreamGreeting out;
    out.set_name("userver");
    for (auto i = 0; i < number; ++i) {
      out.set_number(i);
      EXPECT_NO_THROW(os << out);
      EXPECT_TRUE(os);
    }
    StreamGreeting in;
    EXPECT_NO_THROW(in = os.GetResponse());
    EXPECT_EQ(number, in.number());
    EXPECT_ANY_THROW(os << out);  // TODO Specific exception
  });
}

TEST_F(GrpcClientTest, ClientServerEmptyStream) {
  RunTestInCoro([&] {
    UnitTestServiceClient client{ClientChannel(), GetQueue()};
    auto os = client.WriteMany();
    StreamGreeting in;
    EXPECT_NO_THROW(in = os.GetResponse());
    EXPECT_EQ(0, in.number());
  });
}

TEST_F(GrpcClientTest, BidirStream) {
  RunTestInCoro([&] {
    UnitTestServiceClient client{ClientChannel(), GetQueue()};
    auto number = 42;
    auto bs = client.Chat();

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
  });
}

TEST_F(GrpcClientTest, BidirEmptyStream) {
  RunTestInCoro([&] {
    UnitTestServiceClient client{ClientChannel(), GetQueue()};
    auto bs = client.Chat();
    EXPECT_NO_THROW(bs.FinishWrites());
  });
}

}  // namespace clients::grpc::test
