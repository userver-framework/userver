#include <userver/engine/impl/wait_many_entry.hpp>

#include <engine/impl/wait_any_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

WaitManyEntry::WaitManyEntry(ContextAccessor* context_accessor)
    : context_accessor(context_accessor) {}

WaitManyEntry::~WaitManyEntry() = default;

void WaitManyEntry::Reset() noexcept {
  context_accessor = nullptr;
  wait_scope->reset();
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
