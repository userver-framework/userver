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
constexpr ugrpc::impl::StaticServiceMetadata kGenericMetadataFake{
    kGenericServiceNameFake, kGenericMethodFullNamesFake};

auto Dispatch(
    void (GenericServiceBase::*service_method)(GenericServiceBase::Call&)) {
  return service_method;
}

}  // namespace

template <>
struct CallTraits<void (GenericServiceBase::*)(GenericServiceBase::Call&)>
    final {
  using ServiceBase = GenericServiceBase;
  using Request = grpc::ByteBuffer;
  using Response = grpc::ByteBuffer;
  using RawCall = impl::RawReaderWriter<Request, Response>;
  using InitialRequest = NoInitialRequest;
  using Call = BidirectionalStream<Request, Response>;
  using ContextType = grpc::GenericServerContext;
  using ServiceMethod = void (ServiceBase::*)(Call&);
  static constexpr auto kCallCategory = CallCategory::kGeneric;
};

struct GenericServiceTag final {};

template <>
class AsyncService<GenericServiceTag> final {
 public:
  explicit AsyncService(std::size_t method_count) {
    UASSERT(method_count == 1);
  }

  template <typename CallTraits>
  void Prepare(int method_id, grpc::GenericServerContext& context,
               typename CallTraits::InitialRequest& /*initial_request*/,
               typename CallTraits::RawCall& stream,
               grpc::CompletionQueue& call_cq,
               grpc::ServerCompletionQueue& notification_cq, void* tag) {
    static_assert(CallTraits::kCallCategory == CallCategory::kGeneric);
    UASSERT(method_id == 0);
    service_.RequestCall(&context, &stream, &call_cq, &notification_cq, tag);
  }

  grpc::AsyncGenericService& GetService() { return service_; }

 private:
  grpc::AsyncGenericService service_;
};

struct GenericServiceWorker::Impl {
  Impl(GenericServiceBase& service, ServiceSettings&& settings)
      : service(service),
        service_data(std::move(settings), kGenericMetadataFake) {}

  GenericServiceBase& service;
  ServiceData<GenericServiceTag> service_data;
};

GenericServiceWorker::GenericServiceWorker(GenericServiceBase& service,
                                           ServiceSettings&& settings)
    : impl_(service, std::move(settings)) {}

GenericServiceWorker::GenericServiceWorker(GenericServiceWorker&&) noexcept =
    default;

GenericServiceWorker& GenericServiceWorker::operator=(
    GenericServiceWorker&&) noexcept = default;

GenericServiceWorker::~GenericServiceWorker() = default;

grpc::AsyncGenericService& GenericServiceWorker::GetService() {
  return impl_->service_data.async_service.GetService();
}

void GenericServiceWorker::Start() {
  impl::StartServing(impl_->service_data, impl_->service,
                     Dispatch(&GenericServiceBase::Handle));
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
