#include <userver/ugrpc/client/impl/client_data.hpp>

#include <userver/utils/assert.hpp>

#include <userver/ugrpc/client/impl/completion_queue_pool.hpp>
#include <userver/ugrpc/impl/statistics_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

ClientData::~ClientData() { config_subscription_.Unsubscribe(); }

StubHandle ClientData::NextStub(std::size_t method_id) const {
    auto stub_state = stub_state_.Read();
    auto& stub = impl::NextStub(*stub_state, method_id);
    return StubHandle{std::move(stub_state), stub};
}

StubHandle ClientData::NextStub() const {
    auto stub_state = stub_state_.Read();
    auto& stub = impl::NextStub(*stub_state);
    return StubHandle{std::move(stub_state), stub};
}

grpc::CompletionQueue& ClientData::NextQueue() const { return internals_.completion_queues.NextQueue(); }

ugrpc::impl::MethodStatistics& ClientData::GetStatistics(std::size_t method_id) const {
    UASSERT(service_statistics_);
    return service_statistics_->GetMethodStatistics(method_id);
}

ugrpc::impl::MethodStatistics& ClientData::GetGenericStatistics(std::string_view call_name) const {
    return internals_.statistics_storage.GetGenericStatistics(call_name, internals_.destination_prefix_in_metrics);
}

const ugrpc::impl::StaticServiceMetadata& ClientData::GetMetadata() const {
    UASSERT(metadata_);
    return *metadata_;
}

const dynamic_config::Key<ClientQos>* ClientData::GetClientQos() const { return internals_.qos; }

rcu::ReadablePtr<StubState> ClientData::GetStubState() const { return stub_state_.Read(); }

ugrpc::impl::ServiceStatistics& ClientData::GetServiceStatistics() {
    return internals_.statistics_storage.GetServiceStatistics(GetMetadata(), internals_.destination_prefix_in_metrics);
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
