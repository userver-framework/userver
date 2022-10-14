#pragma once

#include <cstddef>
#include <iterator>

#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/fixed_array.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class ContextAccessor;
class OptionalGenericWaitScope;

using FastPimplOptionalGenericWaitScope =
    utils::FastPimpl<OptionalGenericWaitScope, 80, 16>;

struct WaitManyEntry final {
  explicit WaitManyEntry(ContextAccessor* context_accessor);

  WaitManyEntry(WaitManyEntry&&) = delete;
  WaitManyEntry& operator=(WaitManyEntry&&) = delete;
  ~WaitManyEntry();

  void Reset() noexcept;

  ContextAccessor* context_accessor;
  FastPimplOptionalGenericWaitScope wait_scope;
};

template <typename Container>
utils::FixedArray<WaitManyEntry> ExtractWaitManyEntries(Container& tasks) {
  auto tasks_iter = std::begin(tasks);
  return utils::GenerateFixedArray(
      std::size(tasks), [&](std::size_t /*index*/) {
        auto* const context_accessor = tasks_iter->TryGetContextAccessor();
        ++tasks_iter;
        return WaitManyEntry(context_accessor);
      });
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
