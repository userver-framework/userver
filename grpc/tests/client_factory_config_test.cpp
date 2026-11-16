#include <ugrpc/client/impl/client_factory_config.hpp>

#include <initializer_list>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include <grpc/compression.h>
#include <grpc/grpc.h>
#include <grpc/slice.h>
#include <gtest/gtest.h>

#include <userver/formats/yaml/serialize.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

namespace {

std::optional<int> GetCompressionAlgorithmsBitset(const grpc::ChannelArguments& channel_args) {
    const auto args = channel_args.c_channel_args();
    std::optional<int> result;
    for (const auto& arg : std::span{args.args, args.num_args}) {
        if (arg.type == GRPC_ARG_INTEGER &&
            std::string_view{arg.key} == GRPC_COMPRESSION_CHANNEL_ENABLED_ALGORITHMS_BITSET)
        {
            EXPECT_FALSE(result.has_value());
            result = arg.value.integer;
        }
    }
    return result;
}

}  // namespace

TEST(ClientFactoryConfig, ZstdCompressionEnabledByDefault) {
    grpc_compression_algorithm zstd_algorithm{};
    if (!grpc_compression_algorithm_parse(grpc_slice_from_static_string("zstd"), &zstd_algorithm)) {
        GTEST_SKIP() << "gRPC was built without zstd support";
    }

    const int expected_algorithms =
        (1 << GRPC_COMPRESS_NONE) | (1 << GRPC_COMPRESS_DEFLATE) | (1 << GRPC_COMPRESS_GZIP) | (1 << zstd_algorithm);
    for (const std::string_view yaml : {"{}", "channel-args: {}", "channel-args: {grpc.keepalive_time_ms: 300000}"}) {
        SCOPED_TRACE(yaml);
        const yaml_config::YamlConfig config{formats::yaml::FromString(std::string{yaml}), {}};
        const auto factory_config = config.As<ClientFactoryConfig>();
        EXPECT_EQ(GetCompressionAlgorithmsBitset(factory_config.channel_args), expected_algorithms);
    }
}

TEST(ClientFactoryConfig, ExplicitCompressionAlgorithmsPreserved) {
    const yaml_config::YamlConfig
        config{formats::yaml::FromString("channel-args: {grpc.compression_enabled_algorithms_bitset: 0}"), {}};
    const auto factory_config = config.As<ClientFactoryConfig>();
    EXPECT_EQ(GetCompressionAlgorithmsBitset(factory_config.channel_args), 0);
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
