#include <userver/ugrpc/client/client_factory.hpp>

#include <optional>
#include <stdexcept>

#include <fmt/format.h>

#include <userver/engine/async.hpp>
#include <userver/logging/level_serialization.hpp>
#include <userver/utils/trivial_map.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <ugrpc/impl/logging.hpp>
#include <ugrpc/impl/to_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

namespace {

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

enum class AuthType {
  kInsecure,
  kSsl,
};

AuthType Parse(const yaml_config::YamlConfig& value,
               formats::parse::To<AuthType>) {
  constexpr utils::TrivialBiMap kMap([](auto selector) {
    return selector()
        .Case(AuthType::kInsecure, "insecure")
        .Case(AuthType::kSsl, "ssl");
  });

  return utils::ParseFromValueString(value, kMap);
}

std::shared_ptr<grpc::ChannelCredentials> MakeDefaultCredentials(
    AuthType type) {
  switch (type) {
    case AuthType::kInsecure:
      return grpc::InsecureChannelCredentials();
    case AuthType::kSsl:
      return grpc::SslCredentials({});
  }
  UINVARIANT(false, "Invalid AuthType");
}

}  // namespace

ClientFactoryConfig Parse(const yaml_config::YamlConfig& value,
                          formats::parse::To<ClientFactoryConfig>) {
  ClientFactoryConfig config;
  config.credentials = MakeDefaultCredentials(
      value["auth-type"].As<AuthType>(AuthType::kInsecure));

  config.channel_args =
      MakeChannelArgs(value["channel-args"], value["default-service-config"]);
  config.native_log_level =
      value["native-log-level"].As<logging::Level>(config.native_log_level);
  config.channel_count =
      value["channel-count"].As<std::size_t>(config.channel_count);

  return config;
}

ClientFactory::ClientFactory(ClientFactoryConfig&& config,
                             engine::TaskProcessor& channel_task_processor,
                             grpc::CompletionQueue& queue,
                             utils::statistics::Storage& statistics_storage)
    : channel_task_processor_(channel_task_processor),
      queue_(queue),
      channel_cache_(std::move(config.credentials), config.channel_args,
                     config.channel_count),
      client_statistics_storage_(statistics_storage, "client") {
  ugrpc::impl::SetupNativeLogging();
  ugrpc::impl::UpdateNativeLogLevel(config.native_log_level);
}

impl::ChannelCache::Token ClientFactory::GetChannel(
    const std::string& endpoint) {
  // Spawn a blocking task creating a gRPC channel
  // This is third party code, no use of span inside it
  return engine::AsyncNoSpan(channel_task_processor_,
                             [&] { return channel_cache_.Get(endpoint); })
      .Get();
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
