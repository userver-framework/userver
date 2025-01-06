#include <userver/ugrpc/client/client_factory.hpp>

#include <userver/ugrpc/client/impl/client_data.hpp>
#include <userver/utest/utest.hpp>

#include <tests/service_multichannel.hpp>
#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

using GrpcClientMultichannel = tests::ServiceFixtureMultichannel<sample::ugrpc::UnitTestServiceBase>;

UTEST_P(GrpcClientMultichannel, ChannelsCount) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    auto& data = ugrpc::client::impl::GetClientData(client);
    ASSERT_EQ(data.GetChannelToken().GetChannelCount(), GetParam());
}

INSTANTIATE_UTEST_SUITE_P(/*no prefix*/, GrpcClientMultichannel, testing::Values(std::size_t{1}, std::size_t{4}));

USERVER_NAMESPACE_END
