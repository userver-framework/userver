#include <engine/coro/pool.hpp>

#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::coro {

ucontext_t* GetCoroUcontext(void* sp, std::size_t expected_magic) {
  using Coro = Pool<impl::TaskContext>::Coroutine;

  auto& control_block = *reinterpret_cast<Coro::control_block*>(sp);
  if (control_block.magic != expected_magic) {
    return nullptr;
  }

  if (!control_block.valid() || !control_block.c) {
    return nullptr;
  }

  auto& context =
      (*reinterpret_cast<boost::context::detail::fiber_activation_record**>(
           &control_block.c))
          ->uctx;
  return &context;
}

}  // namespace engine::coro

USERVER_NAMESPACE_END
