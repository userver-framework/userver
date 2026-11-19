#include <userver/ugrpc/client/client_factory.hpp>

#include <grpcpp/security/credentials.h>

#include <userver/ugrpc/client/client_factory_settings.hpp>
#include <userver/ugrpc/client/impl/client_data.hpp>
#include <userver/ugrpc/client/impl/client_data_accessor.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>
#include <userver/utest/utest.hpp>

#include <tests/service_multichannel.hpp>
#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

TEST(GrpcClientFactorySettings, DefaultCredentialsAreResolvedLazily) {
    const ugrpc::client::ClientFactorySettings settings;
    EXPECT_FALSE(settings.credentials);
    EXPECT_TRUE(ugrpc::client::GetClientCredentials(settings, "test"));
}

TEST(GrpcClientFactorySettings, ClientCredentialsOverrideFactoryDefault) {
    const auto default_credentials = grpc::SslCredentials({});
    const auto client_credentials = grpc::InsecureChannelCredentials();
    ugrpc::client::ClientFactorySettings settings;
    settings.credentials = default_credentials;
    settings.client_credentials.emplace("custom", client_credentials);
    EXPECT_EQ(ugrpc::client::GetClientCredentials(settings, "custom"), client_credentials);
    EXPECT_EQ(ugrpc::client::GetClientCredentials(settings, "other"), default_credentials);
}

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
