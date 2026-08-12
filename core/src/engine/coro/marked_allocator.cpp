#include <engine/coro/marked_allocator.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::coro::debug {

// must be accessible from gdb
// without __attribute__((used)) can be optimized out
static const std::size_t page_size __attribute__((used)) = MarkedAllocator::traits_type::page_size();
static std::size_t allocator_stack_size __attribute__((used)) = MarkedAllocator::traits_type::default_size();

MarkedAllocator::MarkedAllocator(std::size_t size) : boost::coroutines2::protected_fixedsize_stack(size) {
    auto alignment = page_size;
    allocator_stack_size = (size + alignment - 1) / alignment * alignment;
}

}  // namespace engine::coro::debug

USERVER_NAMESPACE_END
