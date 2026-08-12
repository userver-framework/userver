#include <userver/engine/impl/context_accessor.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

void WeakAwaitable::ImplMoveWeakAwaitableVirtualTableToCpp() {}

void AwaitableBase::ImplMoveAwaitableBaseVirtualTableToCpp() {}

}  // namespace engine::impl

USERVER_NAMESPACE_END
