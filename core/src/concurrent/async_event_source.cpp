#include <userver/concurrent/async_event_source.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

namespace impl {

AsyncEventSourceBase::~AsyncEventSourceBase() = default;

}  // namespace impl

FunctionId::FunctionId(void* ptr, const std::type_info& type)
    : ptr_(ptr), type_(&type) {
  UASSERT(ptr);
}

FunctionId::operator bool() const { return ptr_ != nullptr; }

bool FunctionId::operator==(const FunctionId& other) const {
  return ptr_ == other.ptr_ && *type_ == *other.type_;
}

std::size_t FunctionId::Hash::operator()(FunctionId id) const noexcept {
  return std::hash<void*>{}(id.ptr_);
}

AsyncEventSubscriberScope::AsyncEventSubscriberScope(
    impl::AsyncEventSourceBase& channel, FunctionId id)
    : channel_(&channel), id_(id) {}

AsyncEventSubscriberScope::AsyncEventSubscriberScope(
    AsyncEventSubscriberScope&& scope) noexcept
    : channel_(scope.channel_), id_(scope.id_) {
  scope.id_ = {};
}

AsyncEventSubscriberScope& AsyncEventSubscriberScope::operator=(
    AsyncEventSubscriberScope&& other) noexcept {
  std::swap(other.channel_, channel_);
  std::swap(other.id_, id_);
  return *this;
}

void AsyncEventSubscriberScope::Unsubscribe() noexcept {
  if (id_) {
    channel_->RemoveListener(id_, UnsubscribingKind::kManual);
    id_ = {};
  }
}

AsyncEventSubscriberScope::~AsyncEventSubscriberScope() {
  if (id_) {
    channel_->RemoveListener(id_, UnsubscribingKind::kAutomatic);
  }
}

}  // namespace concurrent

USERVER_NAMESPACE_END
