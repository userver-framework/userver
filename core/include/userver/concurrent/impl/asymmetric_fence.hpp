#pragma once

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

// A pair of AsymmetricThreadFenceLight + AsymmetricThreadFenceHeavy
// synchronizes like std::atomic_thread_fence(std::memory_order_seq_cst)
//
// Light version is very fast (~1ns) and Heavy version is quite slow
// (1 context switch per CPU core ~1us).
//
// Supported systems: x86_64 Linux with kernel version 4.14+.
// On other systems, these are implemented as
// std::atomic_thread_fence(std::memory_order_seq_cst).
//
// See:
// https://man.archlinux.org/man/membarrier.2.ru
// https://wg21.link/p1202
// https://github.com/facebook/folly/blob/main/folly/synchronization/AsymmetricThreadFence.cpp
void AsymmetricThreadFenceLight() noexcept;
void AsymmetricThreadFenceHeavy() noexcept;

// Automatic thread registration for asymmetric thread fences uses
// an unprotected thread_local access. On thread pools of thread-migrating
// coroutines, this should be called before any AsymmetricThreadFence* calls.
void AsymmetricThreadFenceForceRegisterThread() noexcept;

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
