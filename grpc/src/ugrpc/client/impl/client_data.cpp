#include <userver/ugrpc/client/impl/client_data.hpp>

#include <userver/ugrpc/client/impl/completion_queue_pool.hpp>
#include <userver/ugrpc/impl/statistics_storage.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/rand.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

grpc::CompletionQueue& ClientData::NextQueue() const {
  return params_.completion_queues.NextQueue();
}

ugrpc::impl::MethodStatistics& ClientData::GetStatistics(
    std::size_t method_id) const {
  UASSERT(service_statistics_);
  return service_statistics_->GetMethodStatistics(method_id);
}

ugrpc::impl::MethodStatistics& ClientData::GetGenericStatistics(
    std::string_view call_name) const {
  return params_.statistics_storage.GetGenericStatistics(call_name,
                                                         params_.client_name);
}

const ugrpc::impl::StaticServiceMetadata& ClientData::GetMetadata() const {
  UASSERT(metadata_);
  return *metadata_;
}

const ClientData::StubPtr& ClientData::NextStubPtr() const {
  return stubs_[utils::RandRange(stubs_.size())];
}

ugrpc::impl::ServiceStatistics& ClientData::GetServiceStatistics() {
  return params_.statistics_storage.GetServiceStatistics(GetMetadata(),
                                                         params_.client_name);
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
