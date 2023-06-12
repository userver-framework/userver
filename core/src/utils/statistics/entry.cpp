#include <userver/utils/statistics/entry.hpp>

#include <utility>

#include <userver/logging/log.hpp>

#include <utils/statistics/entry_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

Entry::Entry() = default;

Entry::Entry(const Impl& impl) noexcept : impl_(impl) {}

Entry::Entry(Entry&& other) noexcept
    : impl_(Impl{std::exchange(other.impl_->storage, nullptr),
                 other.impl_->iterator}) {}

Entry& Entry::operator=(Entry&& other) noexcept {
  Unregister();
  std::swap(impl_, other.impl_);
  return *this;
}

Entry::~Entry() {
  if (impl_->storage) {
    impl_->storage->UnregisterExtender(impl_->iterator,
                                       impl::UnregisteringKind::kAutomatic);
  }
}

void Entry::Unregister() noexcept {
  if (impl_->storage) {
    impl_->storage->UnregisterExtender(impl_->iterator,
                                       impl::UnregisteringKind::kManual);
    impl_->storage = nullptr;
  }
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
