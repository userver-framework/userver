#include <ugrpc/client/impl/client_factory_config.hpp>

#include <userver/logging/level_serialization.hpp>
#include <userver/utils/trivial_map.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <ugrpc/client/secdist.hpp>
#include <ugrpc/impl/to_string.hpp>
#include <userver/fs/blocking/read.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

namespace {

std::shared_ptr<grpc::ChannelCredentials> MakeDefaultCredentials(
    impl::AuthType type) {
  switch (type) {
    case AuthType::kInsecure:
      return grpc::InsecureChannelCredentials();
    case AuthType::kSsl:
      return grpc::SslCredentials({});
  }
  UINVARIANT(false, "Invalid AuthType");
}

grpc::ChannelArguments MakeChannelArgs(
    const yaml_config::YamlConfig& channel_args,
    const yaml_config::YamlConfig& default_service_config) {
  grpc::ChannelArguments args;
  if (!channel_args.IsMissing()) {
    for (const auto& [key, value] : Items(channel_args)) {
      if (value.IsInt64()) {
        args.SetInt(ugrpc::impl::ToGrpcString(key), value.As<int>());
      } else {
        args.SetString(ugrpc::impl::ToGrpcString(key), value.As<std::string>());
      }
    }
  }

  if (!default_service_config.IsMissing()) {
    args.SetServiceConfigJSON(default_service_config.As<std::string>());
  }
  return args;
}

}  // namespace

AuthType Parse(const yaml_config::YamlConfig& value,
               formats::parse::To<AuthType>) {
  constexpr utils::TrivialBiMap kMap([](auto selector) {
    return selector()
        .Case(AuthType::kInsecure, "insecure")
        .Case(AuthType::kSsl, "ssl");
  });

  return utils::ParseFromValueString(value, kMap);
}

ClientFactoryConfig Parse(const yaml_config::YamlConfig& value,
                          formats::parse::To<ClientFactoryConfig>) {
  ClientFactoryConfig config;
  config.auth_type = value["auth-type"].As<AuthType>(AuthType::kInsecure);

  /// The buffer containing the PEM encoding of the server root certificates. If
  /// this parameter is empty, the default roots will be used.  The default
  /// roots can be overridden using the \a GRPC_DEFAULT_SSL_ROOTS_FILE_PATH
  /// environment variable pointing to a file on the file system containing the
  /// roots.
  config.pem_root_certs = value["pem-root-certs"].As<std::optional<std::string>>();
  /// The buffer containing the PEM encoding of the client's private key. This
  /// parameter can be empty if the client does not have a private key.
  config.pem_private_key = value["pem-private-key"].As<std::optional<std::string>>();
  /// The buffer containing the PEM encoding of the client's certificate chain.
  /// This parameter can be empty if the client does not have a certificate
  /// chain.
  config.pem_cert_chain = value["pem-cert-chain"].As<std::optional<std::string>>();

  config.channel_args =
      MakeChannelArgs(value["channel-args"], value["default-service-config"]);
  config.native_log_level =
      value["native-log-level"].As<logging::Level>(config.native_log_level);
  config.channel_count =
      value["channel-count"].As<std::size_t>(config.channel_count);

  return config;
}

ClientFactorySettings MakeFactorySettings(
    ClientFactoryConfig&& config,
    const storages::secdist::SecdistConfig* secdist, const testsuite::GrpcControl& testsuite_grpc) {
  std::shared_ptr<grpc::ChannelCredentials> creds;
  if(!testsuite_grpc.IsTlsEnabled()) {
      creds = MakeDefaultCredentials(config.auth_type);
  }
  else
  {
      if(config.auth_type == AuthType::kSsl) {
        grpc::SslCredentialsOptions options;
        if(config.pem_root_certs.has_value())
        {
            options.pem_root_certs = userver::fs::blocking::ReadFileContents(config.pem_root_certs.value());
        }

        if(config.pem_private_key.has_value())
        {
            options.pem_private_key = userver::fs::blocking::ReadFileContents(config.pem_private_key.value());
        }

        if(config.pem_cert_chain.has_value())
        {
            options.pem_cert_chain = userver::fs::blocking::ReadFileContents(config.pem_cert_chain.value());
        }
        creds = grpc::SslCredentials(options);
        LOG_INFO()<<"GRPC client SSL credetials initialized...";
        LOG_INFO()<<"GRPC client SSL pem_root_certs = "<<config.pem_root_certs.value_or("(undefined)");
        LOG_INFO()<<"GRPC client SSL pem_private_key = "<<config.pem_private_key.value_or("(undefined)");
        LOG_INFO()<<"GRPC client SSL pem_cert_chain = "<<config.pem_cert_chain.value_or("(undefined)");
      }
  }

  std::unordered_map<std::string, std::shared_ptr<grpc::ChannelCredentials>>
      client_creds;

  if (secdist) {
    const auto& tokens = secdist->Get<Secdist>();

    for (const auto& [client_name, token] : tokens.tokens) {
      client_creds[client_name] = grpc::CompositeChannelCredentials(
          creds,
          grpc::AccessTokenCredentials(ugrpc::impl::ToGrpcString(token)));
    }
  }

  return ClientFactorySettings{
      creds,
      client_creds,
      config.channel_args,
      config.native_log_level,
      config.channel_count,
  };
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
