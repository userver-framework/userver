#include <userver/utest/utest.hpp>

#include <userver/engine/async.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class UnitTestServiceEcho final : public sample::ugrpc::UnitTestServiceBase {
 public:
  void Chat(ChatCall& call) override {
    sample::ugrpc::StreamGreetingRequest request;
    while (call.Read(request)) {
      call.Write({});
    }
    call.Finish();
  }
};

}  // namespace

using GrpcBidirectionalStream =
    ugrpc::tests::ServiceFixture<UnitTestServiceEcho>;

UTEST_F_MT(GrpcBidirectionalStream, BidirectionalStreamTest, 2) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  auto stream = client.Chat();

  constexpr std::size_t messages_amount = 100000;

  /// [concurrent bidirectional stream]
  auto write_task = engine::AsyncNoSpan([&stream] {
    for (std::size_t i = 0; i < messages_amount; ++i) {
      ASSERT_TRUE(stream.Write({}));
    }

    ASSERT_TRUE(stream.WritesDone());
  });

  sample::ugrpc::StreamGreetingResponse response;
  for (std::size_t i = 0; i < messages_amount; ++i) {
    ASSERT_TRUE(stream.Read(response));
  }
  /// [concurrent bidirectional stream]

  write_task.Get();
}

USERVER_NAMESPACE_END
