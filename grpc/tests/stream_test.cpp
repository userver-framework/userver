#include <userver/utest/utest.hpp>

#include <vector>

#include <userver/engine/async.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class UnitTestServiceEcho final : public sample::ugrpc::UnitTestServiceBase {
public:
    ChatResult Chat(CallContext& /*context*/, ChatReaderWriter& stream) override {
        sample::ugrpc::StreamGreetingRequest request;
        sample::ugrpc::StreamGreetingResponse response{};
        while (stream.Read(request)) {
            stream.Write(response);
        }
        return grpc::Status::OK;
    }
};

}  // namespace

using GrpcBidirectionalStream = ugrpc::tests::ServiceFixture<UnitTestServiceEcho>;

UTEST_F_MT(GrpcBidirectionalStream, BidirectionalStreamTest, 2) {
    constexpr std::size_t kMessagesCount = 200;

    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    auto stream = client.Chat();

    std::vector<sample::ugrpc::StreamGreetingRequest> requests(kMessagesCount);
    std::vector<sample::ugrpc::StreamGreetingResponse> responses;

    /// [concurrent bidirectional stream]
    auto write_task = engine::AsyncNoSpan([&stream, &requests] {
        for (const auto& request : requests) {
            const bool success = stream.Write(request);
            if (!success) return false;
        }

        return stream.WritesDone();
    });

    sample::ugrpc::StreamGreetingResponse response;
    while (stream.Read(response)) {
        responses.push_back(std::move(response));
    }

    ASSERT_TRUE(write_task.Get());
    /// [concurrent bidirectional stream]

    ASSERT_EQ(responses.size(), kMessagesCount);
}

USERVER_NAMESPACE_END
