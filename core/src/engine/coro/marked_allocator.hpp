#pragma once

#include <boost/coroutine2/protected_fixedsize_stack.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::coro::debug {

struct MarkedAllocator : boost::coroutines2::protected_fixedsize_stack {
    MarkedAllocator(std::size_t size = traits_type::default_size());

    const char kCoroutineMark[16] = "ThisIsCoroAlloc";
};

}  // namespace engine::coro::debug

USERVER_NAMESPACE_END
