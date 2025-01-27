#include <userver/ugrpc/client/impl/client_data.hpp>

#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/rand.hpp>

#include <userver/ugrpc/client/client_qos.hpp>
#include <userver/ugrpc/client/impl/completion_queue_pool.hpp>
#include <userver/ugrpc/impl/statistics_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

grpc::CompletionQueue& ClientData::NextQueue() const { return dependencies_.completion_queues.NextQueue(); }

ugrpc::impl::MethodStatistics& ClientData::GetStatistics(std::size_t method_id) const {
    UASSERT(service_statistics_);
    return service_statistics_->GetMethodStatistics(method_id);
}

ugrpc::impl::MethodStatistics& ClientData::GetGenericStatistics(std::string_view call_name) const {
    return dependencies_.statistics_storage.GetGenericStatistics(call_name, dependencies_.client_name);
}

const ugrpc::impl::StaticServiceMetadata& ClientData::GetMetadata() const {
    UASSERT(metadata_);
    return *metadata_;
}

const dynamic_config::Key<ClientQos>* ClientData::GetClientQos() const { return dependencies_.qos; }

const ClientData::StubPtr& ClientData::NextStubPtr(const StubPool& stubs) const {
    return stubs[utils::RandRange(stubs.size())];
}

ugrpc::impl::ServiceStatistics& ClientData::GetServiceStatistics() {
    return dependencies_.statistics_storage.GetServiceStatistics(GetMetadata(), dependencies_.client_name);
}

std::size_t ClientData::GetDedicatedChannelCount(std::size_t method_id) const {
    UASSERT(method_id < dedicated_stubs_.size());
    return dedicated_stubs_[method_id].size();
}

ChannelFactory ClientData::CreateChannelFactory(const ClientDependencies& dependencies) {
    auto credentials = dependencies.testsuite_grpc.IsTlsEnabled()
                           ? GetClientCredentials(dependencies.client_factory_settings, dependencies.client_name)
                           : grpc::InsecureChannelCredentials();
    return ChannelFactory{
        dependencies.channel_task_processor,
        dependencies.endpoint,
        std::move(credentials),
        dependencies.client_factory_settings.channel_args};
}

utils::FixedArray<std::shared_ptr<grpc::Channel>>
ClientData::CreateChannels(const ChannelFactory& channel_factory, std::size_t channel_count) {
    return utils::GenerateFixedArray(channel_count, [&](std::size_t) { return channel_factory.CreateChannel(); });
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
