#include <userver/ugrpc/client/impl/client_data.hpp>

#include <fmt/format.h>
#include <grpcpp/create_channel.h>

#include <userver/ugrpc/client/client_factory.hpp>
#include <userver/ugrpc/client/client_factory_settings.hpp>
#include <userver/ugrpc/client/client_qos.hpp>
#include <userver/ugrpc/client/impl/completion_queue_pool.hpp>
#include <userver/ugrpc/impl/statistics_storage.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/rand.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

namespace {

std::shared_ptr<grpc::ChannelCredentials> GetCredentails(const ClientDependencies& dependencies) {
    return utils::FindOrDefault(
        dependencies.settings.client_credentials, dependencies.client_name, dependencies.settings.credentials
    );
}

bool MethodExists(const ugrpc::impl::StaticServiceMetadata& meta, std::string_view method_name) {
    const auto& rpc_paths = meta.method_full_names;
    return std::find_if(rpc_paths.begin(), rpc_paths.end(), [&method_name, &meta](const std::string_view full_path) {
               const auto method = full_path.substr(meta.service_full_name.size() + 1);
               return method == method_name;
           }) != rpc_paths.end();
}

}  // namespace

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

const ClientData::StubPtr& ClientData::NextStubPtr(const utils::FixedArray<StubPtr>& stubs) const {
    return stubs[utils::RandRange(stubs.size())];
}

ugrpc::impl::ServiceStatistics& ClientData::GetServiceStatistics() {
    return dependencies_.statistics_storage.GetServiceStatistics(GetMetadata(), dependencies_.client_name);
}

std::size_t ClientData::GetDedicatedChannelCount(std::size_t method_id) const {
    UASSERT(method_id < dedicated_stubs_.size());
    return dedicated_stubs_[method_id].size();
}

std::shared_ptr<grpc::Channel>
ClientData::CreateChannelImpl(const ClientDependencies& dependencies, const grpc::string& endpoint) {
    return grpc::CreateCustomChannel(
        endpoint,
        dependencies.testsuite_grpc.IsTlsEnabled() ? GetCredentails(dependencies) : grpc::InsecureChannelCredentials(),
        dependencies.settings.channel_args
    );
}

std::size_t ClientData::GetDedicatedChannelCountImpl(
    const ClientDependencies& dependencies,
    std::size_t method_id,
    const ugrpc::impl::StaticServiceMetadata& meta
) {
    const auto& rpc_paths = meta.method_full_names;
    const auto& rpc_path = rpc_paths[method_id];
    const auto method_name = rpc_path.substr(meta.service_full_name.size() + 1);
    UASSERT(!method_name.empty());

    const auto& dedicated_methods_config = dependencies.dedicated_methods_config;
    const auto it = dedicated_methods_config.find(std::string{method_name});
    if (it != dedicated_methods_config.end()) {
        return it->second;
    }
    UINVARIANT(
        MethodExists(meta, method_name),
        fmt::format("Unknown method {}. Available methods: [{}]", method_name, fmt::join(rpc_paths, ", "))
    );
    return 0;
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
