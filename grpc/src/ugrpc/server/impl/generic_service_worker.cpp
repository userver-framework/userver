#include <ugrpc/server/impl/generic_service_worker.hpp>

#include <userver/ugrpc/server/generic_service_base.hpp>
#include <userver/ugrpc/server/impl/service_worker_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

namespace {

constexpr utils::StringLiteral kGenericServiceFullNameFake = "Generic";

constexpr std::array kGenericMethodsFake = {ugrpc::impl::MethodDescriptor{
    /*method_full_name*/ utils::StringLiteral{"Generic/Generic"},
    /*method_type*/ RpcType::kBidiStreaming,
}};

constexpr ugrpc::impl::StaticServiceMetadata kGenericMetadataFake{kGenericServiceFullNameFake, kGenericMethodsFake};

}  // namespace

struct GenericServiceWorker::Impl {
    Impl(GenericServiceBase& generic_service, ServiceInternals&& internals)
        : generic_service(generic_service),
          generic_service_data(std::move(internals), kGenericMetadataFake)
    {}

    GenericServiceBase& generic_service;
    ServiceData<grpc::AsyncGenericService> generic_service_data;
};

GenericServiceWorker::GenericServiceWorker(GenericServiceBase& generic_service, ServiceInternals&& internals)
    : impl_(generic_service, std::move(internals))
{}

GenericServiceWorker::GenericServiceWorker(GenericServiceWorker&&) noexcept = default;

GenericServiceWorker& GenericServiceWorker::operator=(GenericServiceWorker&&) noexcept = default;

GenericServiceWorker::~GenericServiceWorker() = default;

grpc::AsyncGenericService& GenericServiceWorker::GetService() {
    return impl_->generic_service_data.async_service.GetAsyncGenericService();
}

void GenericServiceWorker::Start() {
    impl::StartProcessing(impl_->generic_service_data, impl_->generic_service, &GenericServiceBase::Handle);
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
