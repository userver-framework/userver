#include <userver/utest/utest.hpp>

#include <userver/tracing/span.hpp>
#include <userver/utils/algo.hpp>

#include <ugrpc/impl/rpc_metadata_keys.hpp>
#include <ugrpc/impl/to_string.hpp>

#include <tests/service_fixture_test.hpp>
#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

template <typename MetadataMap>
grpc::string GetMetadata(const MetadataMap& metadata, const grpc::string& key) {
  return ugrpc::impl::ToGrpcString(utils::FindOrDefault(metadata, key));
}

const grpc::string kServerTraceId = "server-trace-id";
const grpc::string kServerSpanId = "server-span-id";
const grpc::string kServerLink = "server-link";
const grpc::string kServerParentSpanId = "server-parent-span-id";
const grpc::string kServerParentLink = "server-parent-link";
const grpc::string kClientTraceIdEcho = "client-trace-id-echo";
const grpc::string kClientSpanIdEcho = "client-span-id-echo";
const grpc::string kClientLinkEcho = "client-link-echo";

class UnitTestServiceWithTracingChecks final
    : public sample::ugrpc::UnitTestServiceBase {
 public:
  void SayHello(SayHelloCall& call, sample::ugrpc::GreetingRequest&&) override {
    SetMetadata(call.GetContext());
    call.Finish({});
  }

  void ReadMany(ReadManyCall& call,
                sample::ugrpc::StreamGreetingRequest&&) override {
    SetMetadata(call.GetContext());
    call.Finish();
  }

  void WriteMany(WriteManyCall& call) override {
    SetMetadata(call.GetContext());
    call.Finish({});
  }

  void Chat(ChatCall& call) override {
    SetMetadata(call.GetContext());
    call.Finish();
  }

 private:
  static void SetMetadata(grpc::ServerContext& context) {
    const auto& span = tracing::Span::CurrentSpan();
    const auto& client_meta = context.client_metadata();

    context.AddInitialMetadata(kServerTraceId,
                               ugrpc::impl::ToGrpcString(span.GetTraceId()));
    context.AddInitialMetadata(kServerSpanId,
                               ugrpc::impl::ToGrpcString(span.GetSpanId()));
    context.AddInitialMetadata(kServerLink,
                               ugrpc::impl::ToGrpcString(span.GetLink()));
    context.AddInitialMetadata(kServerParentSpanId,
                               ugrpc::impl::ToGrpcString(span.GetParentId()));
    context.AddInitialMetadata(kServerParentLink,
                               ugrpc::impl::ToGrpcString(span.GetParentLink()));

    context.AddInitialMetadata(
        kClientTraceIdEcho, GetMetadata(client_meta, ugrpc::impl::kXYaTraceId));
    context.AddInitialMetadata(
        kClientSpanIdEcho, GetMetadata(client_meta, ugrpc::impl::kXYaSpanId));
    context.AddInitialMetadata(
        kClientLinkEcho, GetMetadata(client_meta, ugrpc::impl::kXYaRequestId));
  }
};

using GrpcTracing = GrpcServiceFixtureSimple<UnitTestServiceWithTracingChecks>;

void CheckMetadata(const grpc::ClientContext& context) {
  const auto& metadata = context.GetServerInitialMetadata();
  const auto& span = tracing::Span::CurrentSpan();

  // - TraceId should propagate both to sub-spans within a single service,
  //   and from client to server
  // - Link should propagate within a single service, but not from client to
  //   server (a new link will be generated for the request processing task)
  // - SpanId should not propagate
  // - there are also ParentSpanId and ParentLink with obvious semantics
  // - client uses a detached sub-Span for the RPC

  // the checks below follow EXPECT_EQ(cause, effect) order
  EXPECT_EQ(span.GetTraceId(), GetMetadata(metadata, kClientTraceIdEcho));
  EXPECT_EQ(GetMetadata(metadata, kClientTraceIdEcho),
            GetMetadata(metadata, kServerTraceId));

  EXPECT_NE(span.GetSpanId(), GetMetadata(metadata, kClientSpanIdEcho));
  EXPECT_EQ(GetMetadata(metadata, kClientSpanIdEcho),
            GetMetadata(metadata, kServerParentSpanId));
  EXPECT_NE(GetMetadata(metadata, kServerParentSpanId),
            GetMetadata(metadata, kServerSpanId));

  EXPECT_EQ(span.GetLink(), GetMetadata(metadata, kClientLinkEcho));
  EXPECT_EQ(GetMetadata(metadata, kClientLinkEcho),
            GetMetadata(metadata, kServerParentLink));
  EXPECT_NE(GetMetadata(metadata, kServerParentLink),
            GetMetadata(metadata, kServerLink));
}

}  // namespace

UTEST_F(GrpcTracing, UnaryRPC) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  sample::ugrpc::GreetingRequest out;
  out.set_name("userver");
  auto call = client.SayHello(out);
  UEXPECT_NO_THROW(call.Finish());
  CheckMetadata(call.GetContext());
}

UTEST_F(GrpcTracing, InputStream) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  sample::ugrpc::StreamGreetingRequest out;
  out.set_name("userver");
  out.set_number(42);
  sample::ugrpc::StreamGreetingResponse in;
  auto call = client.ReadMany(out);
  EXPECT_FALSE(call.Read(in));
  CheckMetadata(call.GetContext());
}

UTEST_F(GrpcTracing, OutputStream) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  auto call = client.WriteMany();
  UEXPECT_NO_THROW(call.Finish());
  CheckMetadata(call.GetContext());
}

UTEST_F(GrpcTracing, BidirectionalStream) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  sample::ugrpc::StreamGreetingResponse in;
  auto call = client.Chat();
  EXPECT_FALSE(call.Read(in));
  CheckMetadata(call.GetContext());
}

UTEST_F(GrpcTracing, SpansInDifferentRPCs) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  sample::ugrpc::GreetingRequest out;
  out.set_name("userver");

  auto call1 = client.SayHello(out);
  call1.Finish();
  const auto& metadata1 = call1.GetContext().GetServerInitialMetadata();

  auto call2 = client.SayHello(out);
  call2.Finish();
  const auto& metadata2 = call2.GetContext().GetServerInitialMetadata();

  EXPECT_EQ(GetMetadata(metadata1, kServerTraceId),
            GetMetadata(metadata2, kServerTraceId));
  EXPECT_NE(GetMetadata(metadata1, kServerSpanId),
            GetMetadata(metadata2, kServerSpanId));
  EXPECT_NE(GetMetadata(metadata1, kServerParentSpanId),
            GetMetadata(metadata2, kServerParentSpanId));
  EXPECT_NE(GetMetadata(metadata1, kServerLink),
            GetMetadata(metadata2, kServerLink));
  EXPECT_EQ(GetMetadata(metadata1, kServerParentLink),
            GetMetadata(metadata2, kServerParentLink));
}

USERVER_NAMESPACE_END
