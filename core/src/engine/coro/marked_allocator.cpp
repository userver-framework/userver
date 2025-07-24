#include <engine/coro/marked_allocator.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::coro::debug {

static volatile const std::size_t page_size = MarkedAllocator::traits_type::page_size();
static volatile std::size_t allocator_stack_size = MarkedAllocator::traits_type::default_size();

MarkedAllocator::MarkedAllocator(std::size_t size) : boost::coroutines2::protected_fixedsize_stack(size) {
    auto aligment = page_size;
    allocator_stack_size = (size + aligment - 1) / aligment * aligment;
}

}  // namespace engine::coro::debug

USERVER_NAMESPACE_END
