#include <ugrpc/client/impl/client_factory_config.hpp>

#include <userver/formats/yaml/serialize.hpp>
#include <userver/logging/level_serialization.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/trivial_map.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <ugrpc/client/secdist.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/logging/log.hpp>
#include <userver/ugrpc/impl/to_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

namespace {

std::shared_ptr<grpc::ChannelCredentials> MakeCredentials(const ClientFactoryConfig& config) {
    if (config.auth_type == AuthType::kSsl) {
        LOG_INFO() << "GRPC client (SSL) initialized...";
        return grpc::SslCredentials(config.ssl_credentials_options);
    } else {
        LOG_INFO() << "GRPC client (non SSL) initialized...";
        return grpc::InsecureChannelCredentials();
    }
}

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
        LOG_INFO() << "Set client ChannelArguments: "
                   << formats::yaml::ToString(channel_args.As<formats::yaml::Value>());
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

AuthType Parse(const yaml_config::YamlConfig& value, formats::parse::To<AuthType>) {
    constexpr utils::TrivialBiMap kMap([](auto selector) {
        return selector().Case(AuthType::kInsecure, "insecure").Case(AuthType::kSsl, "ssl");
    });

    return utils::ParseFromValueString(value, kMap);
}

ClientFactoryConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<ClientFactoryConfig>) {
    ClientFactoryConfig config;
    config.auth_type = value["auth-type"].As<AuthType>(AuthType::kInsecure);
    if (config.auth_type == AuthType::kSsl) {
        config.ssl_credentials_options = MakeCredentialsOptions(value["ssl-credentials-options"]);
    }
    config.channel_args = MakeChannelArgs(value["channel-args"]);
    config.default_service_config = value["default-service-config"].As<std::optional<std::string>>();
    config.channel_count = value["channel-count"].As<std::size_t>(config.channel_count);
    return config;
}

ClientFactorySettings
MakeFactorySettings(ClientFactoryConfig&& config, const storages::secdist::SecdistConfig* secdist) {
    std::shared_ptr<grpc::ChannelCredentials> credentials = MakeCredentials(config);
    std::unordered_map<std::string, std::shared_ptr<grpc::ChannelCredentials>> client_credentials;

    if (secdist) {
        const auto& tokens = secdist->Get<Secdist>();

        for (const auto& [client_name, token] : tokens.tokens) {
            client_credentials[client_name] = grpc::CompositeChannelCredentials(
                credentials, grpc::AccessTokenCredentials(ugrpc::impl::ToGrpcString(token))
            );
        }
    }

    return ClientFactorySettings{
        std::move(credentials),
        std::move(client_credentials),
        std::move(config.channel_args),
        std::move(config.default_service_config),
        config.channel_count,
    };
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
