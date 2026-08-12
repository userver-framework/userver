#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

#include <grpcpp/completion_queue.h>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/testsuite/grpc_control.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fixed_array.hpp>

#include <userver/ugrpc/client/client_qos.hpp>
#include <userver/ugrpc/client/impl/channel_argument_utils.hpp>
#include <userver/ugrpc/client/impl/client_internals.hpp>
#include <userver/ugrpc/client/impl/client_qos_errors_reporter.hpp>
#include <userver/ugrpc/client/impl/compat/channel_arguments_builder.hpp>
#include <userver/ugrpc/client/impl/stub_state.hpp>
#include <userver/ugrpc/client/middlewares/fwd.hpp>
#include <userver/ugrpc/impl/async_service.hpp>
#include <userver/ugrpc/impl/static_service_metadata.hpp>
#include <userver/ugrpc/impl/statistics.hpp>

#include <dynamic_config/variables/EGRESS_GRPC_PROXY_ENABLED.hpp>
#include <dynamic_config/variables/EGRESS_NO_PROXY_TARGETS.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

struct StubState;

struct GenericClientTag final {
    explicit GenericClientTag() = default;
};

/// The internal state of generated gRPC clients
class ClientData final {
public:
    ClientData() = delete;

    template <typename Service>
    ClientData(ClientInternals&& internals, ugrpc::impl::StaticServiceMetadata metadata, std::in_place_type_t<Service>)
        : internals_(std::move(internals)),
          metadata_(metadata),
          service_statistics_(&GetServiceStatistics()),
          channel_arguments_builder_(
              std::in_place,
              internals_.channel_args,
              internals_.default_service_config,
              internals_.retry_config,
              metadata
          ),
          method_retry_limiters_(
              CreateRetryLimiters(internals_.retry_limiter_factory, metadata, internals_.destination_prefix_in_metrics)
          )
    {
        if (internals_.qos) {
            SubscribeOnConfigUpdate<Service>(
                ::dynamic_config::EGRESS_GRPC_PROXY_ENABLED,
                ::dynamic_config::EGRESS_NO_PROXY_TARGETS,
                *internals_.qos
            );
        } else {
            SubscribeOnConfigUpdate<
                Service>(::dynamic_config::EGRESS_GRPC_PROXY_ENABLED, ::dynamic_config::EGRESS_NO_PROXY_TARGETS);
        }
    }

    template <typename Service>
    ClientData(ClientInternals&& internals, GenericClientTag, std::in_place_type_t<Service>)
        : internals_(std::move(internals))
    {
        SubscribeOnConfigUpdate<
            Service>(::dynamic_config::EGRESS_GRPC_PROXY_ENABLED, ::dynamic_config::EGRESS_NO_PROXY_TARGETS);
    }

    ~ClientData();

    ClientData(ClientData&&) noexcept = delete;
    ClientData(const ClientData&) = delete;
    ClientData& operator=(ClientData&&) = delete;
    ClientData& operator=(const ClientData&) = delete;

    grpc::CompletionQueue& NextQueue() const;

    dynamic_config::Snapshot GetConfigSnapshot() const { return internals_.config_source.GetSnapshot(); }

    ugrpc::impl::MethodStatistics& GetStatistics(std::size_t method_id) const;

    ugrpc::impl::MethodStatistics& GetGenericStatistics(std::string_view call_name) const;

    std::string_view GetClientName() const { return internals_.client_name; }

    const Middlewares& GetMiddlewares() const { return internals_.middlewares; }

    const ugrpc::impl::StaticServiceMetadata& GetMetadata() const;

    const testsuite::GrpcControl& GetTestsuiteControl() const { return internals_.testsuite_grpc; }

    const dynamic_config::Key<ClientQos>* GetClientQos() const;

    const RetryConfig& GetRetryConfig() const { return internals_.retry_config; }

    RetryLimiterFactory* GetRetryLimiterFactory() const noexcept { return internals_.retry_limiter_factory; }

    RetryLimiter* GetRetryLimiter(std::size_t method_id) const noexcept;

    rcu::ReadablePtr<StubState> GetStubState() const;

    /// @returns Target endpoint address string from the channel factory
    std::string_view GetEndpoint() const { return internals_.endpoint; }

    std::string_view GetDestinationPrefixInMetrics() const { return internals_.destination_prefix_in_metrics; }

private:
    template <typename Service>
    static StubArray MakeStubs(
        std::size_t size,
        const ChannelFactory& channel_factory,
        std::string_view target,
        const grpc::ChannelArguments& channel_args
    ) {
        auto channels = utils::GenerateFixedArray(size, [&channel_factory, target, &channel_args](std::size_t) {
            return channel_factory.CreateChannel(target, channel_args);
        });
        auto stubs = utils::GenerateFixedArray(channels.size(), [&channels](std::size_t index) {
            return ugrpc::impl::AsyncService<Service>::NewStub(channels[index]);
        });
        return StubArray{std::move(channels), std::move(stubs)};
    }

    template <typename Service>
    static utils::FixedArray<StubArray> MakeDedicatedStubs(
        const ugrpc::impl::StaticServiceMetadata& metadata,
        const DedicatedMethodsConfig& dedicated_methods_config,
        const ChannelFactory& channel_factory,
        std::string_view target,
        const grpc::ChannelArguments& channel_args
    ) {
        return utils::GenerateFixedArray(GetMethodsCount(metadata), [&](std::size_t method_id) {
            const auto method_channel_count =
                GetMethodChannelCount(dedicated_methods_config, GetMethodName(metadata, method_id));
            return MakeStubs<Service>(method_channel_count, channel_factory, target, channel_args);
        });
    }

    static utils::FixedArray<std::unique_ptr<RetryLimiter>> CreateRetryLimiters(
        RetryLimiterFactory* factory,
        const ugrpc::impl::StaticServiceMetadata& metadata,
        std::string_view destination_prefix_in_metrics
    );

    ugrpc::impl::ServiceStatistics& GetServiceStatistics();

    template <typename Service, typename... Keys>
    void SubscribeOnConfigUpdate(const Keys&... keys) {
        config_subscription_ =
            internals_.config_source
                .UpdateAndListen(this, internals_.client_name, &ClientData::OnConfigUpdate<Service>, keys...);
    }

    template <typename Service>
    void OnConfigUpdate(const dynamic_config::Snapshot& config) {
        const auto& client_qos = internals_.qos ? config[*internals_.qos] : ClientQos{};

        if (metadata_.has_value() && internals_.qos) {
            internals_.client_qos_error_reporter
                .ValidateAndReportClientQosErrors(client_qos, internals_.qos->GetName(), *metadata_);
        }

        std::string target = internals_.endpoint;

        auto channel_args =
            channel_arguments_builder_.has_value()
                ? channel_arguments_builder_->Build(client_qos)
                : BuildChannelArguments(internals_.channel_args, internals_.default_service_config);

        const auto& proxy_settings = internals_.proxy_settings;

        auto proxy_enabled = config[::dynamic_config::EGRESS_GRPC_PROXY_ENABLED];
        const auto& no_proxy_targets = config[::dynamic_config::EGRESS_NO_PROXY_TARGETS].targets;
        if (!proxy_settings.proxy_address.empty() && proxy_enabled && !proxy_settings.no_proxy_targets.count(target) &&
            !no_proxy_targets.count(target))
        {
            SetHttpProxy(target, channel_args, internals_.channel_factory.GetAuthType(), proxy_settings.proxy_address);
        }
        auto stubs = MakeStubs<Service>(internals_.channel_count, internals_.channel_factory, target, channel_args);

        auto dedicated_stubs =
            metadata_.has_value()
                ? MakeDedicatedStubs<Service>(
                      *metadata_,
                      internals_.dedicated_methods_config,
                      internals_.channel_factory,
                      target,
                      channel_args
                  )
                : utils::FixedArray<StubArray>{};

        stub_state_.Assign({client_qos, std::move(stubs), std::move(dedicated_stubs)});
    }

    ClientInternals internals_;
    std::optional<ugrpc::impl::StaticServiceMetadata> metadata_;
    ugrpc::impl::ServiceStatistics* service_statistics_{nullptr};

    std::optional<compat::ChannelArgumentsBuilder> channel_arguments_builder_;

    rcu::Variable<StubState> stub_state_;

    // RetryLimiter instances per method
    utils::FixedArray<std::unique_ptr<RetryLimiter>> method_retry_limiters_;

    // These fields must be the last ones
    concurrent::AsyncEventSubscriberScope config_subscription_;
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
