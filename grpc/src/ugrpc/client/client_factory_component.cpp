#include <userver/ugrpc/client/client_factory_component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/ugrpc/client/common_component.hpp>
#include <userver/ugrpc/client/middlewares/pipeline.hpp>
#include <userver/ugrpc/impl/to_string.hpp>

#include <ugrpc/client/impl/client_factory_config.hpp>
#include <ugrpc/client/secdist.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/ugrpc/client/client_factory_component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

namespace {

const storages::secdist::SecdistConfig* GetSecdistConfig(const components::ComponentContext& context) {
    if (const auto* const component = context.FindComponentOptional<components::Secdist>()) {
        return &component->Get();
    }
    return nullptr;
}

std::shared_ptr<grpc::ChannelCredentials> MakeCredentials(
    AuthType auth_type,
    const grpc::SslCredentialsOptions& ssl_credentials_options
) {
    if (auth_type == AuthType::kSsl) {
        LOG_INFO() << "GRPC client (SSL) initialized...";
        return grpc::SslCredentials(ssl_credentials_options);
    } else {
        LOG_INFO() << "GRPC client (non SSL) initialized...";
        return grpc::InsecureChannelCredentials();
    }
}

std::unordered_map<std::string, std::shared_ptr<grpc::ChannelCredentials>> MakeClientCredentials(
    const std::shared_ptr<grpc::ChannelCredentials>& credentials,
    const storages::secdist::SecdistConfig* secdist_config
) {
    std::unordered_map<std::string, std::shared_ptr<grpc::ChannelCredentials>> client_credentials;

    if (secdist_config) {
        const auto& secdist = secdist_config->Get<Secdist>();
        for (const auto& [client_name, token] : secdist.tokens) {
            client_credentials[client_name] = grpc::CompositeChannelCredentials(
                credentials,
                grpc::AccessTokenCredentials(ugrpc::impl::ToGrpcString(token))
            );
        }
    }

    return client_credentials;
}

// Looks up a RetryLimiterFactory component by name. Used for both this
// factory's own explicit `retry-limiter` override and the common
// (grpc-client-common-level) shared default -- see
// CommonComponent::retry_limiter_factory_name_ for why the latter is looked
// up lazily here rather than eagerly in CommonComponent's own constructor.
RetryLimiterFactory* FindRetryLimiterFactory(
    const std::optional<std::string>& retry_limiter_factory_name,
    const components::ComponentContext& context
) {
    if (!retry_limiter_factory_name) {
        return nullptr;
    }

    auto* retry_limiter_factory = context.FindComponentOptional<RetryLimiterFactory>(*retry_limiter_factory_name);
    UINVARIANT(
        retry_limiter_factory,
        fmt::format("RetryLimiterFactory component '{}' not found", *retry_limiter_factory_name)
    );
    return retry_limiter_factory;
}

bool ShouldUseConstantDynamicConfigs(const components::ComponentConfig& config) {
    return config["use-constant-dynamic-configs"].As<bool>(false);
}

// Explicit sentinel value for `retry-limiter`, forcing no retry-limiter for this factory regardless of any common
// default. This is orthogonal to `use-constant-dynamic-configs`: an omitted (or `null`) `retry-limiter` falls back
// to the common default depending on `use-constant-dynamic-configs`, while `none` always disables it.
constexpr std::string_view kNoRetryLimiterSentinel = "none";

bool IsNoRetryLimiterSentinel(const std::optional<std::string>& own_retry_limiter_factory_name) {
    return own_retry_limiter_factory_name == kNoRetryLimiterSentinel;
}

}  // namespace

ClientFactoryComponent::ClientFactoryComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : impl::MiddlewareRunnerComponentBase(config, context, MiddlewarePipelineComponent::kName) {
    auto& client_common_component = context.FindComponent<CommonComponent>();

    const bool use_constant_dynamic_configs = ShouldUseConstantDynamicConfigs(config);

    const auto own_retry_limiter_factory_name = config["retry-limiter"].As<std::optional<std::string>>();
    const bool no_retry_limiter = IsNoRetryLimiterSentinel(own_retry_limiter_factory_name);

    RetryLimiterFactory* retry_limiter_factory = nullptr;
    if (!no_retry_limiter) {
        retry_limiter_factory = FindRetryLimiterFactory(own_retry_limiter_factory_name, context);
        if (!retry_limiter_factory && !use_constant_dynamic_configs) {
            retry_limiter_factory =
                FindRetryLimiterFactory(client_common_component.retry_limiter_factory_name_, context);
        }
    }

    auto& metrics_storage = context.FindComponent<components::StatisticsStorage>().GetMetricsStorageRef();

    auto& dynamic_config_component = context.FindComponent<components::DynamicConfig>();
    const auto config_source =
        use_constant_dynamic_configs
            ? dynamic_config_component.GetDefaultsAsConstantSource()
            : dynamic_config_component.GetSource();

    auto& testsuite_grpc = context.FindComponent<components::TestsuiteSupport>().GetGrpcControl();
    auto client_factory_config = config.As<impl::ClientFactoryConfig>();
    if (!testsuite_grpc.IsTlsEnabled() && client_factory_config.auth_type == AuthType::kSsl) {
        LOG_INFO() << "Disabling TLS/SSL dues to testsuite config for gRPC";
        client_factory_config.auth_type = AuthType::kInsecure;
    }

    auto credentials = MakeCredentials(client_factory_config.auth_type, client_factory_config.ssl_credentials_options);

    auto client_credentials = MakeClientCredentials(credentials, GetSecdistConfig(context));

    ClientFactorySettings client_factory_settings{
        client_factory_config.auth_type,
        std::move(credentials),
        std::move(client_credentials),
        std::move(client_factory_config.retry_config),
        retry_limiter_factory,
        std::move(client_factory_config.channel_args),
        std::move(client_factory_config.default_service_config),
        client_factory_config.channel_count,
        client_common_component.proxy_settings_,
    };

    factory_.emplace(
        std::move(client_factory_settings),
        client_common_component.blocking_task_processor_,
        *this,  // impl::PipelineCreatorInterface&
        client_common_component.completion_queues_,
        client_common_component.client_statistics_storage_,
        metrics_storage,
        testsuite_grpc,
        config_source
    );
}

ClientFactory& ClientFactoryComponent::GetFactory() { return *factory_; }

yaml_config::Schema ClientFactoryComponent::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<
        impl::MiddlewareRunnerComponentBase>("src/ugrpc/client/client_factory_component.yaml");
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
