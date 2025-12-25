#include <ugrpc/client/impl/client_factory_config.hpp>

#include <userver/formats/yaml/serialize.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/trivial_map.hpp>

#include <userver/ugrpc/impl/to_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

namespace {

grpc::SslCredentialsOptions MakeCredentialsOptions(const yaml_config::YamlConfig& config) {
    grpc::SslCredentialsOptions result;
    if (config.IsMissing()) {
        return result;
    }

    if (!config["pem-root-certs"].IsMissing()) {
        result.pem_root_certs = fs::blocking::ReadFileContents(config["pem-root-certs"].As<std::string>());
    }
    if (!config["pem-private-key"].IsMissing()) {
        result.pem_private_key = fs::blocking::ReadFileContents(config["pem-private-key"].As<std::string>());
    }
    if (!config["pem-cert-chain"].IsMissing()) {
        result.pem_cert_chain = fs::blocking::ReadFileContents(config["pem-cert-chain"].As<std::string>());
    }

    return result;
}

grpc::ChannelArguments MakeChannelArgs(const yaml_config::YamlConfig& channel_args) {
    grpc::ChannelArguments args;
    if (!channel_args.IsMissing()) {
        LOG_INFO()
            << "Set client ChannelArguments: " << formats::yaml::ToString(channel_args.As<formats::yaml::Value>());
        for (const auto& [key, value] : Items(channel_args)) {
            if (value.IsInt64()) {
                args.SetInt(ugrpc::impl::ToGrpcString(key), value.As<int>());
            } else {
                args.SetString(ugrpc::impl::ToGrpcString(key), value.As<std::string>());
            }
        }
    }
    return args;
}

}  // namespace

ClientFactoryConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<ClientFactoryConfig>) {
    ClientFactoryConfig config;
    config.auth_type = value["auth-type"].As<AuthType>(AuthType::kInsecure);
    if (config.auth_type == AuthType::kSsl) {
        config.ssl_credentials_options = MakeCredentialsOptions(value["ssl-credentials-options"]);
    }
    config.retry_config = value["retry-config"].As<RetryConfig>();
    config.channel_args = MakeChannelArgs(value["channel-args"]);
    config.default_service_config = value["default-service-config"].As<std::optional<std::string>>();
    config.channel_count = value["channel-count"].As<std::size_t>(config.channel_count);
    return config;
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
