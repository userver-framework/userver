#include <gtest/gtest.h>

#include <userver/utils/algo.hpp>
#include <userver/utils/from_string.hpp>

#include <userver/ugrpc/impl/to_string.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

#include <ugrpc/impl/grpc_string_logging.hpp>
#include <ugrpc/impl/rpc_metadata.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class UnitTestService final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext& context, sample::ugrpc::GreetingRequest&& /*request*/) override {
        const auto prev_attempts =
            utils::FindOptional(context.GetServerContext().client_metadata(), ugrpc::impl::kXPrevAttempts);

        if (++attempt_ == 1) {
            LOG_DEBUG() << attempt_ << ": prev_attempts=" << prev_attempts;
            EXPECT_FALSE(prev_attempts.has_value());
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, "");
        }

        LOG_DEBUG() << attempt_ << ": prev_attempts=" << prev_attempts;
        EXPECT_TRUE(prev_attempts.has_value());
        EXPECT_EQ(attempt_ - 1, utils::FromString<int>(ugrpc::impl::ToStringView(prev_attempts.value())));

        if (attempt_ < 4) {
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, "");
        }

        return sample::ugrpc::GreetingResponse{};
    }

private:
    std::uint64_t attempt_{};
};

using RetryMetadataTest = ugrpc::tests::ServiceWithClientFixture<UnitTestService, sample::ugrpc::UnitTestServiceClient>;

}  // namespace

UTEST_F(RetryMetadataTest, PrevAttempts) {
    ugrpc::client::CallOptions call_options;
    call_options.SetAttempts(4);
    UEXPECT_NO_THROW(GetClient().SayHello(sample::ugrpc::GreetingRequest{}, std::move(call_options)));
}

USERVER_NAMESPACE_END
