#include <ugrpc/server/impl/generic_service_worker.hpp>

#include <array>
#include <utility>

#include <userver/utils/assert.hpp>

#include <userver/ugrpc/server/generic_service_base.hpp>
#include <userver/ugrpc/server/impl/service_worker_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

namespace {

constexpr utils::StringLiteral kGenericServiceFullNameFake = "Generic";

constexpr std::array kGenericMethodsFake = {ugrpc::impl::MethodDescriptor{
    /*method_full_name*/ utils::StringLiteral{"/Generic/Generic"},
    /*method_type*/ RpcType::kBidiStreaming,
}};

constexpr ugrpc::impl::StaticServiceMetadata kGenericMetadataFake{kGenericServiceFullNameFake, kGenericMethodsFake};

using GenericCallTraits = CallTraits<decltype(&GenericServiceBase::Handle)>;
using GenericCallbackCallTraits = CallbackCallTraits<GenericCallTraits>;

class CallbackGenericService final : public grpc::CallbackGenericService {
public:
    CallbackGenericService(
        GenericServiceBase& generic_service,
        ServiceData<grpc::AsyncGenericService>& generic_service_data
    )
        : method_data_{generic_service_data, 0, generic_service, &GenericServiceBase::Handle}
    {}

    grpc::ServerGenericBidiReactor* CreateReactor(grpc::GenericCallbackServerContext* server_context) override {
        UASSERT(server_context);
        return new Reactor{method_data_, *server_context, no_serialized_initial_request, no_final_response};
    }

private:
    MethodData<grpc::AsyncGenericService, GenericCallbackCallTraits> method_data_;
};

}  // namespace

struct GenericServiceWorker::Impl {
    Impl(GenericServiceBase& generic_service, ServiceInternals&& internals)
        : generic_service(generic_service),
          generic_service_data(std::move(internals), kGenericMetadataFake),
          callback_generic_service(generic_service, generic_service_data)
    {}

    GenericServiceBase& generic_service;
    ServiceData<grpc::AsyncGenericService> generic_service_data;
    CallbackGenericService callback_generic_service;
};

GenericServiceWorker::GenericServiceWorker(GenericServiceBase& generic_service, ServiceInternals&& internals)
    : impl_(generic_service, std::move(internals))
{}

GenericServiceWorker::GenericServiceWorker(GenericServiceWorker&&) noexcept = default;

GenericServiceWorker& GenericServiceWorker::operator=(GenericServiceWorker&&) noexcept = default;

GenericServiceWorker::~GenericServiceWorker() = default;

grpc::AsyncGenericService& GenericServiceWorker::GetAsyncService() {
    return impl_->generic_service_data.async_service.GetAsyncGenericService();
}

grpc::CallbackGenericService& GenericServiceWorker::GetCallbackService() { return impl_->callback_generic_service; }

void GenericServiceWorker::Start() {
    UASSERT(!impl_->generic_service_data.internals.use_callback_api);
    impl::StartProcessing(impl_->generic_service_data, impl_->generic_service, &GenericServiceBase::Handle);
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
