#include <userver/ugrpc/server/generic_service_base.hpp>

#include <userver/ugrpc/server/impl/call_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

GenericServiceBase::~GenericServiceBase() = default;

GenericServiceBase::GenericResult GenericServiceBase::Handle(
    GenericCallContext& /*context*/, GenericReaderWriter& /*stream*/) {
  UASSERT_MSG(
      false,
      "Called not implemented GenericServiceBase/Handle(GenericCallContext&, "
      "GenericReaderWriter&)");
  return USERVER_NAMESPACE::ugrpc::server::impl::kUnimplementedStatus;
}

void GenericServiceBase::Handle(Call& call) {
  GenericCallContext context{call};
  auto result = Handle(context, call);
  impl::Finish(call, std::move(result));
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
