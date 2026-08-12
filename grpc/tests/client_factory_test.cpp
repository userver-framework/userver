#include <userver/ugrpc/client/client_factory.hpp>

#include <userver/ugrpc/client/impl/client_data.hpp>
#include <userver/ugrpc/client/impl/client_data_accessor.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>
#include <userver/utest/utest.hpp>

#include <tests/service_multichannel.hpp>
#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

using GrpcClientMultichannel = tests::ServiceFixtureMultichannel<sample::ugrpc::UnitTestServiceBase>;

UTEST_P(GrpcClientMultichannel, ChannelsCount) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    const auto& data = ugrpc::client::impl::ClientDataAccessor::GetClientData(client);
    const auto stub_state = data.GetStubState();
    ASSERT_EQ(stub_state->stubs.stubs.size(), GetParam());
}

INSTANTIATE_UTEST_SUITE_P(/*no prefix*/, GrpcClientMultichannel, testing::Values(std::size_t{1}, std::size_t{4}));

using GrpcClientDeathTest = ugrpc::tests::ServiceFixture<sample::ugrpc::UnitTestServiceBase>;

UTEST_F_DEATH(GrpcClientDeathTest, ClientDieAfterFactory) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    EXPECT_DEATH(StopServer(), "Some clients are still alive");
}

USERVER_NAMESPACE_END
