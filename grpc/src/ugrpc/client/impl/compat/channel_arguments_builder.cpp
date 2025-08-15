#include <userver/ugrpc/client/impl/compat/channel_arguments_builder.hpp>

#include <fmt/format.h>

#include <google/protobuf/util/json_util.h>
#include <google/protobuf/util/time_util.h>

#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/numeric_cast.hpp>

#include <userver/ugrpc/client/client_qos.hpp>
#include <userver/ugrpc/client/impl/channel_argument_utils.hpp>
#include <userver/ugrpc/impl/to_string.hpp>

#include <ugrpc/client/impl/compat/retry_policy.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl::compat {

namespace {

void SetName(formats::json::ValueBuilder& method_config, formats::json::Value name) {
    method_config["name"] = std::move(name);
}

formats::json::Value Normalize(const formats::json::Value& name, const formats::json::Value& method_config) {
    formats::json::ValueBuilder method_config_builder{method_config};
    SetName(method_config_builder, formats::json::MakeArray(name));
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
                            default_method_config = Normalize(name, method_config);
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
                        default_method_config = Normalize(name, method_config);
                        continue;
                    }

                    const auto method_id = FindMethod(metadata, service_name, method_name);
                    if (!method_id.has_value()) {
                        throw std::runtime_error{
                            fmt::format("Invalid MethodConfig: unknown method name {}", method_name)};
                    }

                    const auto [_, inserted] = method_configs.try_emplace(*method_id, Normalize(name, method_config));
                    UINVARIANT(inserted, "Each name entry must be unique across the entire ServiceConfig");
                }
            }
        }
    }

    return {std::move(method_configs), std::move(default_method_config)};
}

class RetryConfiguration final {
public:
    RetryConfiguration(
        const ugrpc::impl::StaticServiceMetadata& metadata,
        const RetryConfig& retry_config,
        const ServiceConfigBuilder::PreparedMethodConfigs& prepared_method_configs,
        const ClientQos& client_qos
    )
        : metadata_{metadata},
          retry_config_{retry_config},
          prepared_method_configs_{prepared_method_configs},
          client_qos_{client_qos} {}

    bool HasMethodConfiguration(size_t method_id) {
        // method Qos exists and has non empty 'attempts' value
        const auto method_full_name = GetMethodFullName(metadata_, method_id);
        if (client_qos_.methods.HasValue(method_full_name)) {
            const auto& qos = client_qos_.methods.Get(method_full_name);
            if (qos.attempts.has_value()) {
                return true;
            }
        }

        // or `static-service-config` has corresponding MethodConfig entry
        return prepared_method_configs_.method_configs.count(method_id);
    }

    bool HasDefaultConfiguration() { return GetDefaultAttempts().has_value(); }

    std::optional<int> GetAttempts(size_t method_id) {
        const auto method_full_name = GetMethodFullName(metadata_, method_id);
        const auto qos = client_qos_.methods.GetOptional(method_full_name);
        if (qos.has_value() && qos->attempts.has_value()) {
            return qos->attempts;
        }

        return GetDefaultAttempts();
    }

    std::optional<int> GetDefaultAttempts() {
        if (client_qos_.methods.HasDefaultValue()) {
            const auto qos_default = client_qos_.methods.GetDefaultValue();
            if (qos_default.attempts.has_value()) {
                return qos_default.attempts;
            }
        }

        return GetStaticConfigAttempts();
    }

    std::optional<formats::json::Value> GetMethodConfig(size_t method_id) {
        auto method_config = utils::FindOptional(prepared_method_configs_.method_configs, method_id);
        return method_config.has_value() ? method_config : GetDefaultMethodConfig();
    }

    std::optional<formats::json::Value> GetDefaultMethodConfig() {
        return prepared_method_configs_.default_method_config;
    }

private:
    std::optional<int> GetStaticConfigAttempts() const {
        // for RetryConfig 'attempts == 1' means stay ServiceConfig AS IS
        if (1 == retry_config_.attempts) {
            return std::nullopt;
        }

        return retry_config_.attempts;
    }

    const ugrpc::impl::StaticServiceMetadata& metadata_;
    const RetryConfig& retry_config_;
    const ServiceConfigBuilder::PreparedMethodConfigs& prepared_method_configs_;
    const ClientQos& client_qos_;
};

void ClearRetryPolicy(formats::json::ValueBuilder& method_config) {
    method_config.Remove("retryPolicy");
    method_config.Remove("hedgingPolicy");
}

struct RetryPolicy {
    std::uint32_t max_attempts{};
    std::optional<std::chrono::milliseconds> per_attempt_recv_timeout;
};
void SetRetryPolicy(formats::json::ValueBuilder& method_config, const RetryPolicy& retry_policy) {
    UINVARIANT(1 < retry_policy.max_attempts, "RetryPolicy MaxAttempts must be greater than 1");

    // Only one of "retryPolicy" or "hedgingPolicy" may be set
    if (method_config.HasMember("hedgingPolicy")) {
        method_config.Remove("hedgingPolicy");
    }

    if (!method_config.HasMember("retryPolicy")) {
        method_config["retryPolicy"] = ConstructDefaultRetryPolicy();
    }
    method_config["retryPolicy"]["maxAttempts"] = retry_policy.max_attempts;

    // do not set "perAttemptRecvTimeout" for now
    // https://github.com/grpc/grpc/issues/39935
    // "Client request may hangs forever when set 'perAttemptRecvTimeout'"
    //
    // if (retry_policy.per_attempt_recv_timeout.has_value()) {
    //     method_config["retryPolicy"]["perAttemptRecvTimeout"] =
    //         ugrpc::impl::ToString(google::protobuf::util::TimeUtil::ToString(
    //             google::protobuf::util::TimeUtil::MillisecondsToDuration(retry_policy.per_attempt_recv_timeout->count())
    //         ));
    // }
}

void ApplyAttempts(formats::json::ValueBuilder& method_config, int attempts) {
    UINVARIANT(0 < attempts, "'attempts' value must be greater than 0");
    if (1 == attempts) {
        ClearRetryPolicy(method_config);
    } else {
        SetRetryPolicy(method_config, RetryPolicy{utils::numeric_cast<std::uint32_t>(attempts), std::nullopt});
    }
}

class MethodConfigBuilder final {
public:
    void SetMethodConfig(std::optional<formats::json::Value> method_config) {
        method_config_ = std::move(method_config);
    }

    void SetName(std::string_view service, std::string_view method) { SetName(service, std::vector{method}); }

    void SetName(std::string_view service, const std::vector<std::string_view>& methods) {
        formats::json::ValueBuilder name{};
        for (const auto& method : methods) {
            UINVARIANT(
                !service.empty() || method.empty(), "If the 'service' field is empty, the 'method' field must be empty"
            );
            name.PushBack(formats::json::MakeObject("service", service, "method", method));
        }
        name_ = name.ExtractValue();
    }

    void SetAttempts(std::optional<int> attempts) { attempts_ = attempts; }

    formats::json::Value Build() && {
        formats::json::ValueBuilder method_config_builder{
            std::move(method_config_).value_or(formats::json::MakeObject())};

        if (name_.has_value()) {
            impl::compat::SetName(method_config_builder, std::move(*name_));
        }

        if (attempts_.has_value()) {
            ApplyAttempts(method_config_builder, *attempts_);
        }

        return method_config_builder.ExtractValue();
    }

private:
    std::optional<formats::json::Value> method_config_;
    std::optional<formats::json::Value> name_;
    std::optional<int> attempts_;
};

}  // namespace

ServiceConfigBuilder::ServiceConfigBuilder(
    const ugrpc::impl::StaticServiceMetadata& metadata,
    const RetryConfig& retry_config,
    const std::optional<std::string>& static_service_config
)
    : metadata_{metadata}, retry_config_{retry_config} {
    LOG_INFO() << "ServiceConfigBuilder, RetryConfig: attempts=" << retry_config_.attempts;

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

    // push unary method configs AS IS
    for (const auto& [method_id, method_config] : prepared_method_configs_.method_configs) {
        if (ugrpc::impl::RpcType::kUnary == GetMethodType(metadata_, method_id)) {
            method_config_array.PushBack(method_config);
        }
    }

    RetryConfiguration retry_configuration{metadata_, retry_config_, prepared_method_configs_, client_qos};

    std::vector<std::string_view> default_stream_methods;

    for (std::size_t method_id = 0; method_id < GetMethodsCount(metadata_); ++method_id) {
        if (ugrpc::impl::RpcType::kUnary == GetMethodType(metadata_, method_id)) {
            continue;
        }

        if (retry_configuration.HasMethodConfiguration(method_id)) {
            MethodConfigBuilder builder;
            builder.SetMethodConfig(retry_configuration.GetMethodConfig(method_id));
            builder.SetName(metadata_.service_full_name, GetMethodName(metadata_, method_id));
            builder.SetAttempts(retry_configuration.GetAttempts(method_id));
            method_config_array.PushBack(std::move(builder).Build());
            continue;
        }

        default_stream_methods.push_back(GetMethodName(metadata_, method_id));
    }

    // add default MethodConfig for streaming methods
    if (!default_stream_methods.empty() && retry_configuration.HasDefaultConfiguration()) {
        MethodConfigBuilder builder;
        builder.SetMethodConfig(retry_configuration.GetDefaultMethodConfig());
        builder.SetName(metadata_.service_full_name, default_stream_methods);
        builder.SetAttempts(retry_configuration.GetDefaultAttempts());
        method_config_array.PushBack(std::move(builder).Build());
    }

    // push default MethodConfig AS IS
    if (prepared_method_configs_.default_method_config.has_value()) {
        method_config_array.PushBack(prepared_method_configs_.default_method_config);
    }

    return method_config_array.ExtractValue();
}

ChannelArgumentsBuilder::ChannelArgumentsBuilder(
    const grpc::ChannelArguments& channel_args,
    const std::optional<std::string>& static_service_config,
    const RetryConfig& retry_config,
    const ugrpc::impl::StaticServiceMetadata& metadata
)
    : channel_args_{channel_args}, service_config_builder_{metadata, retry_config, static_service_config} {}

grpc::ChannelArguments ChannelArgumentsBuilder::Build(const ClientQos& client_qos) const {
    const auto service_config = service_config_builder_.Build(client_qos);
    if (service_config.IsNull()) {
        return channel_args_;
    }
    return BuildChannelArguments(channel_args_, formats::json::ToString(service_config));
}

}  // namespace ugrpc::client::impl::compat

USERVER_NAMESPACE_END
