#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

#include <grpcpp/completion_queue.h>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/testsuite/grpc_control.hpp>
#include <userver/utils/fixed_array.hpp>

#include <userver/ugrpc/client/impl/client_internals.hpp>
#include <userver/ugrpc/client/impl/stub_any.hpp>
#include <userver/ugrpc/client/impl/stub_pool.hpp>
#include <userver/ugrpc/client/middlewares/fwd.hpp>
#include <userver/ugrpc/impl/static_service_metadata.hpp>
#include <userver/ugrpc/impl/statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

struct GenericClientTag final {
    explicit GenericClientTag() = default;
};

/// The internal state of generated gRPC clients
class ClientData final {
public:
    struct StubState {
        StubPool stubs;
        // method_id -> stub_pool
        utils::FixedArray<StubPool> dedicated_stubs;
    };

    class StubHandle {
    public:
        StubHandle(rcu::ReadablePtr<StubState>&& state, StubAny& stub) : state_{std::move(state)}, stub_{stub} {}

        StubHandle(StubHandle&&) noexcept = default;
        StubHandle& operator=(StubHandle&&) = delete;

        StubHandle(const StubHandle&) = delete;
        StubHandle& operator=(const StubHandle&) = delete;

        template <typename Stub>
        Stub& Get() {
            return StubCast<Stub>(stub_);
        }

    private:
        rcu::ReadablePtr<StubState> state_;
        StubAny& stub_;
    };

    ClientData() = delete;

    template <typename Service>
    ClientData(ClientInternals&& internals, ugrpc::impl::StaticServiceMetadata metadata, std::in_place_type_t<Service>)
        : internals_(std::move(internals)),
          metadata_(metadata),
          service_statistics_(&GetServiceStatistics()),
          stub_state_(std::make_unique<rcu::Variable<StubState>>()) {
        if (internals_.qos) {
            SubscribeOnConfigUpdate<Service>(*internals_.qos);
        } else {
            ConstructStubState<Service>(internals_.channel_arguments_builder.Build());
        }
    }

    template <typename Service>
    ClientData(ClientInternals&& internals, GenericClientTag, std::in_place_type_t<Service>)
        : internals_(std::move(internals)), stub_state_(std::make_unique<rcu::Variable<StubState>>()) {
        ConstructStubState<Service>(internals_.channel_arguments_builder.Build());
    }

    ~ClientData();

    ClientData(ClientData&&) = default;
    ClientData& operator=(ClientData&&) = delete;

    ClientData(const ClientData&) = delete;
    ClientData& operator=(const ClientData&) = delete;

    StubHandle NextStubFromMethodId(std::size_t method_id) const {
        auto stub_state = stub_state_->Read();
        auto& dedicated_stubs = stub_state->dedicated_stubs[method_id];
        auto& stubs = dedicated_stubs.Size() ? dedicated_stubs : stub_state->stubs;
        auto& stub = stubs.NextStub();
        return StubHandle{std::move(stub_state), stub};
    }

    StubHandle NextStub() const {
        auto stub_state = stub_state_->Read();
        auto& stub = stub_state->stubs.NextStub();
        return StubHandle{std::move(stub_state), stub};
    }

    grpc::CompletionQueue& NextQueue() const;

    dynamic_config::Snapshot GetConfigSnapshot() const { return internals_.config_source.GetSnapshot(); }

    ugrpc::impl::MethodStatistics& GetStatistics(std::size_t method_id) const;

    ugrpc::impl::MethodStatistics& GetGenericStatistics(std::string_view call_name) const;

    std::string_view GetClientName() const { return internals_.client_name; }

    const Middlewares& GetMiddlewares() const { return internals_.mws; }

    const ugrpc::impl::StaticServiceMetadata& GetMetadata() const;

    const testsuite::GrpcControl& GetTestsuiteControl() const { return internals_.testsuite_grpc; }

    const dynamic_config::Key<ClientQos>* GetClientQos() const;

    rcu::ReadablePtr<StubState> GetStubState() const { return stub_state_->Read(); }

private:
    template <typename Stub>
    static utils::FixedArray<StubPool> MakeDedicatedStubs(
        const ugrpc::impl::StaticServiceMetadata& metadata,
        const DedicatedMethodsConfig& dedicated_methods_config,
        const ChannelFactory& channel_factory,
        const grpc::ChannelArguments& channel_args
    ) {
        return utils::GenerateFixedArray(GetMethodsCount(metadata), [&](std::size_t method_id) {
            const auto method_channel_count =
                GetMethodChannelCount(dedicated_methods_config, GetMethodName(metadata, method_id));
            return StubPool::Create<Stub>(method_channel_count, channel_factory, channel_args);
        });
    }

    ugrpc::impl::ServiceStatistics& GetServiceStatistics();

    template <typename Service>
    void SubscribeOnConfigUpdate(const dynamic_config::Key<ClientQos>& qos) {
        config_subscription_ = internals_.config_source.UpdateAndListen(
            this, internals_.client_name, &ClientData::OnConfigUpdate<Service>, qos
        );
    }

    template <typename Service>
    void OnConfigUpdate(const dynamic_config::Snapshot& config) {
        UASSERT(internals_.qos);
        const auto& client_qos = config[*internals_.qos];
        ConstructStubState<Service>(internals_.channel_arguments_builder.Build(client_qos));
    }

    template <typename Service>
    void ConstructStubState(const grpc::ChannelArguments& channel_args) {
        auto stubs = StubPool::Create<typename Service::Stub>(
            internals_.channel_count, internals_.channel_factory, channel_args
        );

        auto dedicated_stubs =
            metadata_.has_value()
                ? MakeDedicatedStubs<typename Service::Stub>(
                      *metadata_, internals_.dedicated_methods_config, internals_.channel_factory, channel_args
                  )
                : utils::FixedArray<StubPool>{};

        stub_state_->Assign({std::move(stubs), std::move(dedicated_stubs)});
    }

    ClientInternals internals_;
    std::optional<ugrpc::impl::StaticServiceMetadata> metadata_{std::nullopt};
    ugrpc::impl::ServiceStatistics* service_statistics_{nullptr};

    std::unique_ptr<rcu::Variable<StubState>> stub_state_;

    // These fields must be the last ones
    concurrent::AsyncEventSubscriberScope config_subscription_;
};

template <typename Client>
ClientData& GetClientData(Client& client) {
    return client.impl_;
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
