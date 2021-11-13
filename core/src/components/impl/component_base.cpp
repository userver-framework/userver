#include <userver/components/impl/component_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace components::impl {

// Putting detructor into a cpp file to force vtable instantiation in only 1
// translation unit
ComponentBase::~ComponentBase() = default;

}  // namespace components::impl

USERVER_NAMESPACE_END
