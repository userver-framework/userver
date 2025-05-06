#include <ugrpc/server/impl/generic_service_worker.hpp>

#include <grpcpp/generic/async_generic_service.h>

#include <userver/ugrpc/server/generic_service_base.hpp>
#include <userver/ugrpc/server/impl/call_traits.hpp>
#include <userver/ugrpc/server/impl/service_worker_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

namespace {

constexpr std::string_view kGenericMethodFullNamesFake[] = {
    "Generic/Generic",
};
constexpr std::string_view kGenericServiceNameFake = "Generic";
constexpr ugrpc::impl::StaticServiceMetadata kGenericMetadataFake{kGenericServiceNameFake, kGenericMethodFullNamesFake};

}  // namespace

struct GenericServiceTag final {};

template <>
class AsyncService<GenericServiceTag> final {
public:
    explicit AsyncService(std::size_t method_count) { UASSERT(method_count == 1); }

    template <typename CallTraits>
    void Prepare(
        int method_id,
        grpc::GenericServerContext& context,
        typename CallTraits::InitialRequest& /*initial_request*/,
        typename CallTraits::RawResponder& responder,
        grpc::CompletionQueue& call_cq,
        grpc::ServerCompletionQueue& notification_cq,
        void* tag
    ) {
        UASSERT(method_id == 0);
        service_.RequestCall(&context, &responder, &call_cq, &notification_cq, tag);
    }

    grpc::AsyncGenericService& GetService() { return service_; }

private:
    grpc::AsyncGenericService service_;
};

struct GenericServiceWorker::Impl {
    Impl(GenericServiceBase& service, ServiceInternals&& internals)
        : service(service), service_data(std::move(internals), kGenericMetadataFake) {}

    GenericServiceBase& service;
    ServiceData<GenericServiceTag> service_data;
};

GenericServiceWorker::GenericServiceWorker(GenericServiceBase& service, ServiceInternals&& internals)
    : impl_(service, std::move(internals)) {}

GenericServiceWorker::GenericServiceWorker(GenericServiceWorker&&) noexcept = default;

GenericServiceWorker& GenericServiceWorker::operator=(GenericServiceWorker&&) noexcept = default;

GenericServiceWorker::~GenericServiceWorker() = default;

grpc::AsyncGenericService& GenericServiceWorker::GetService() { return impl_->service_data.async_service.GetService(); }

void GenericServiceWorker::Start() {
    impl::StartServing(impl_->service_data, impl_->service, &GenericServiceBase::Handle);
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
