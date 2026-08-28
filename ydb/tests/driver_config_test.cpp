#include <userver/formats/yaml.hpp>
#include <userver/utest/utest.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <ydb/impl/config.hpp>
#include <ydb/impl/secdist.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

ydb::impl::DriverSettings DriverSettingsFromYaml(std::string_view yaml) {
    const yaml_config::YamlConfig config{formats::yaml::FromString(std::string{yaml}), {}};
    ydb::impl::secdist::DatabaseSettings secdist;
    secdist.endpoint = "localhost:2136";
    secdist.database = "local";
    return ydb::impl::ParseDriverSettings(config, secdist, nullptr);
}

}  // namespace

UTEST(YdbDriverConfig, GrpcCompressionAlgorithmGzip) {
    const auto settings = DriverSettingsFromYaml("grpc-compression-algorithm: gzip");
    ASSERT_TRUE(settings.grpc_compression_algorithm.has_value());
    EXPECT_EQ(*settings.grpc_compression_algorithm, NYdb::EGrpcCompressionAlgorithm::Gzip);
}

UTEST(YdbDriverConfig, GrpcCompressionAlgorithmDeflate) {
    const auto settings = DriverSettingsFromYaml("grpc-compression-algorithm: deflate");
    ASSERT_TRUE(settings.grpc_compression_algorithm.has_value());
    EXPECT_EQ(*settings.grpc_compression_algorithm, NYdb::EGrpcCompressionAlgorithm::Deflate);
}

UTEST(YdbDriverConfig, GrpcCompressionAlgorithmNone) {
    const auto settings = DriverSettingsFromYaml("grpc-compression-algorithm: none");
    ASSERT_TRUE(settings.grpc_compression_algorithm.has_value());
    EXPECT_EQ(*settings.grpc_compression_algorithm, NYdb::EGrpcCompressionAlgorithm::None);
}

UTEST(YdbDriverConfig, GrpcCompressionAlgorithmUnknownThrows) {
    EXPECT_THROW(DriverSettingsFromYaml("grpc-compression-algorithm: brotli"), yaml_config::Exception);
}

UTEST(YdbDriverConfig, GrpcLoadBalancingPolicyRoundRobin) {
    const auto settings = DriverSettingsFromYaml("grpc-load-balancing-policy: round_robin");
    ASSERT_TRUE(settings.grpc_load_balancing_policy.has_value());
    EXPECT_EQ(*settings.grpc_load_balancing_policy, "round_robin");
}

UTEST(YdbDriverConfig, GrpcLoadBalancingPolicyPickFirst) {
    const auto settings = DriverSettingsFromYaml("grpc-load-balancing-policy: pick_first");
    ASSERT_TRUE(settings.grpc_load_balancing_policy.has_value());
    EXPECT_EQ(*settings.grpc_load_balancing_policy, "pick_first");
}

UTEST(YdbDriverConfig, GrpcLoadBalancingPolicyUnknownThrows) {
    EXPECT_THROW(DriverSettingsFromYaml("grpc-load-balancing-policy: random"), yaml_config::Exception);
}

UTEST(YdbDriverConfig, MissingKeysLeaveSettingsUnset) {
    const auto settings = DriverSettingsFromYaml("max_pool_size: 10");
    EXPECT_FALSE(settings.grpc_compression_algorithm.has_value());
    EXPECT_FALSE(settings.grpc_load_balancing_policy.has_value());
}

USERVER_NAMESPACE_END
