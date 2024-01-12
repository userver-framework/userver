#include <userver/compiler/thread_local.hpp>

#include <cstddef>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace compiler::impl {

namespace {

std::size_t& CoroutineSwitchBansCount() noexcept {
  return ThreadLocal([] { return std::size_t{0}; });
}

}  // namespace

bool AreCoroutineSwitchesAllowed() noexcept {
  return CoroutineSwitchBansCount() == 0;
}

void IncrementLocalCoroutineSwitchBans() noexcept {
  ++CoroutineSwitchBansCount();
}

void DecrementLocalCoroutineSwitchBans() noexcept {
  auto& block_count = CoroutineSwitchBansCount();
  UASSERT(block_count != 0);
  --block_count;
}

}  // namespace compiler::impl

USERVER_NAMESPACE_END
