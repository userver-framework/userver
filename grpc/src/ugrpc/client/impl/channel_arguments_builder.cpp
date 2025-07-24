#include <userver/ugrpc/client/impl/channel_arguments_builder.hpp>

#include <fmt/format.h>

#include <google/protobuf/util/json_util.h>
#include <google/protobuf/util/time_util.h>

#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>

#include <userver/ugrpc/client/client_qos.hpp>
#include <userver/ugrpc/impl/to_string.hpp>

#include <ugrpc/client/impl/retry_policy.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

namespace {

constexpr bool HasValue(const Qos& qos) noexcept { return qos.attempts.has_value(); }

void SetName(formats::json::ValueBuilder& method_config, formats::json::Value name) {
    method_config["name"] = formats::json::MakeArray(std::move(name));
}

void ApplyQosConfig(formats::json::ValueBuilder& method_config, const Qos& qos) {
    const auto attempts = GetAttempts(qos);
    if (attempts.has_value()) {
        method_config.Remove("hedgingPolicy");

        if (1 == *attempts) {
            method_config.Remove("retryPolicy");
            return;
        }

        if (!method_config.HasMember("retryPolicy")) {
            method_config["retryPolicy"] = ConstructDefaultRetryPolicy();
        }
        method_config["retryPolicy"]["maxAttempts"] = *attempts;
        if (qos.timeout.has_value()) {
            method_config["retryPolicy"]["perAttemptRecvTimeout"] =
                ugrpc::impl::ToString(google::protobuf::util::TimeUtil::ToString(
                    google::protobuf::util::TimeUtil::MillisecondsToDuration(qos.timeout->count())
                ));
        }
    }
}

formats::json::Value ApplyQosConfig(formats::json::Value method_config, const Qos& qos) {
    formats::json::ValueBuilder method_config_builder{std::move(method_config)};
    ApplyQosConfig(method_config_builder, qos);
    return method_config_builder.ExtractValue();
}

formats::json::Value Normalize(const formats::json::Value& method_config, formats::json::Value name) {
    formats::json::ValueBuilder method_config_builder{method_config};
    SetName(method_config_builder, std::move(name));
    return method_config_builder.ExtractValue();
}

formats::json::Value NormalizeDefault(const formats::json::Value& method_config) {
    formats::json::ValueBuilder method_config_builder{method_config};
    SetName(method_config_builder, formats::json::MakeObject());
    return method_config_builder.ExtractValue();
}

ServiceConfigBuilder::PreparedMethodConfigs PrepareMethodConfigs(
    const formats::json::Value& static_service_config,
    const ugrpc::impl::StaticServiceMetadata& metadata
) {
    std::unordered_map<std::size_t, formats::json::Value> method_configs;
    std::optional<formats::json::Value> default_method_config;

    if (static_service_config.HasMember("methodConfig")) {
        for (const auto& method_config : static_service_config["methodConfig"]) {
            if (method_config.HasMember("name")) {
                for (const auto& name : method_config["name"]) {
                    const auto service_name = name["service"].As<std::string>("");
                    const auto method_name = name["method"].As<std::string>("");

                    // - If the 'service' field is empty, the 'method' field must be empty, and
                    //   this MethodConfig specifies the default for all methods (it's the default
                    //   config).
                    if (service_name.empty()) {
                        UINVARIANT(
                            method_name.empty(), "If the 'service' field is empty, the 'method' field must be empty"
                        );
                        if (!default_method_config.has_value()) {
                            default_method_config = NormalizeDefault(method_config);
                        }
                        continue;
                    }

                    if (metadata.service_full_name != service_name) {
                        throw std::runtime_error{
                            fmt::format("Invalid MethodConfig: unknown service name {}", service_name)};
                    }

                    // - If the 'method' field is empty, this MethodConfig specifies the defaults
                    //   for all methods for the specified service.
                    if (method_name.empty()) {
                        default_method_config = NormalizeDefault(method_config);
                        continue;
                    }

                    const auto method_id = FindMethod(metadata, service_name, method_name);
                    if (!method_id.has_value()) {
                        throw std::runtime_error{
                            fmt::format("Invalid MethodConfig: unknown method name {}", method_name)};
                    }

                    const auto [_, inserted] = method_configs.try_emplace(*method_id, Normalize(method_config, name));
                    UINVARIANT(inserted, "Each name entry must be unique across the entire ServiceConfig");
                }
            }
        }
    }

    return {std::move(method_configs), std::move(default_method_config)};
}

formats::json::Value
BuildMethodConfig(formats::json::Value name, const Qos& qos, std::optional<formats::json::Value> static_method_config) {
    formats::json::ValueBuilder method_config{std::move(static_method_config).value_or(formats::json::MakeObject())};

    SetName(method_config, std::move(name));

    ApplyQosConfig(method_config, qos);

    return method_config.ExtractValue();
}

formats::json::Value BuildMethodConfig(
    std::string_view service_name,
    std::string_view method_name,
    const Qos& qos,
    std::optional<formats::json::Value> static_method_config
) {
    UINVARIANT(
        !service_name.empty() || method_name.empty(),
        "If the 'service' field is empty, the 'method' field must be empty"
    );

    auto name = formats::json::MakeObject("service", service_name, "method", method_name);

    return BuildMethodConfig(std::move(name), qos, std::move(static_method_config));
}

formats::json::Value
BuildDefaultMethodConfig(const Qos& qos, std::optional<formats::json::Value> static_method_config) {
    return BuildMethodConfig(formats::json::MakeObject(), qos, std::move(static_method_config));
}

}  // namespace

ServiceConfigBuilder::ServiceConfigBuilder(
    const ugrpc::impl::StaticServiceMetadata& metadata,
    const std::optional<std::string>& static_service_config
)
    : metadata_{metadata} {
    if (static_service_config.has_value()) {
        static_service_config_ = formats::json::FromString(*static_service_config);
        prepared_method_configs_ = PrepareMethodConfigs(static_service_config_, metadata_);
    }
}

formats::json::Value ServiceConfigBuilder::Build(const ClientQos& client_qos) const {
    formats::json::ValueBuilder service_config_builder{static_service_config_};

    if (auto method_config_array = BuildMethodConfigArray(client_qos); !method_config_array.IsEmpty()) {
        service_config_builder["methodConfig"] = std::move(method_config_array);
    }

    return service_config_builder.ExtractValue();
}

formats::json::Value ServiceConfigBuilder::BuildMethodConfigArray(const ClientQos& client_qos) const {
    formats::json::ValueBuilder method_config_array;

    const auto qos_default =
        client_qos.methods.HasDefaultValue() ? client_qos.methods.GetDefaultValue() : std::optional<Qos>{};

    auto default_method_config = prepared_method_configs_.default_method_config;
    if (default_method_config.has_value() && qos_default.has_value()) {
        default_method_config = ApplyQosConfig(std::move(*default_method_config), *qos_default);
    }

    for (std::size_t method_id = 0; method_id < GetMethodsCount(metadata_); ++method_id) {
        const auto method_name = GetMethodName(metadata_, method_id);
        const auto method_full_name = GetMethodFullName(metadata_, method_id);

        const auto qos = client_qos.methods.GetOptional(method_full_name);

        auto method_config = utils::FindOptional(prepared_method_configs_.method_configs, method_id);
        if (method_config.has_value() && qos_default.has_value()) {
            method_config = ApplyQosConfig(std::move(*method_config), *qos_default);
        }

        // add MethodConfig
        // if method Qos with non empty value exists,
        // or `static-service-config` has corresponding MethodConfig entry
        if (client_qos.methods.HasValue(method_full_name) && HasValue(*qos)) {
            method_config_array.PushBack(BuildMethodConfig(
                metadata_.service_full_name,
                method_name,
                *qos,
                method_config.has_value() ? std::move(method_config) : default_method_config
            ));
        } else if (method_config.has_value()) {
            method_config_array.PushBack(std::move(*method_config));
        }
    }

    // add default MethodConfig if default Qos with non empty value exists
    // or `static-service-config` has default MethodConfig
    if (qos_default.has_value() && HasValue(*qos_default)) {
        method_config_array.PushBack(BuildDefaultMethodConfig(*qos_default, std::move(default_method_config)));
    } else if (default_method_config.has_value()) {
        method_config_array.PushBack(std::move(*default_method_config));
    }

    return method_config_array.ExtractValue();
}

ChannelArgumentsBuilder::ChannelArgumentsBuilder(
    const grpc::ChannelArguments& channel_args,
    const std::optional<std::string>& static_service_config,
    const ugrpc::impl::StaticServiceMetadata& metadata
)
    : channel_args_{channel_args}, service_config_builder_{metadata, static_service_config} {}

grpc::ChannelArguments ChannelArgumentsBuilder::Build(const ClientQos& client_qos) const {
    const auto service_config = service_config_builder_.Build(client_qos);
    if (service_config.IsNull()) {
        return channel_args_;
    }
    return BuildChannelArguments(channel_args_, formats::json::ToString(service_config));
}

grpc::ChannelArguments
BuildChannelArguments(const grpc::ChannelArguments& channel_args, const std::optional<std::string>& service_config) {
    if (!service_config.has_value()) {
        return channel_args;
    }

    LOG_INFO() << "Building ChannelArguments, ServiceConfig: " << *service_config;
    auto effective_channel_args{channel_args};
    effective_channel_args.SetServiceConfigJSON(ugrpc::impl::ToGrpcString(*service_config));
#ifdef GRPC_ARG_EXPERIMENTAL_ENABLE_HEDGING
    effective_channel_args.SetInt(GRPC_ARG_EXPERIMENTAL_ENABLE_HEDGING, 1);
#endif  // GRPC_ARG_EXPERIMENTAL_ENABLE_HEDGING
    return effective_channel_args;
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
