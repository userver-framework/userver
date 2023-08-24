
//          Copyright Oliver Kowalke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#if defined(BOOST_USE_UCONTEXT)
#include <uboost_coro/context/continuation_ucontext.hpp>
#elif defined(BOOST_USE_WINFIB)
#include <uboost_coro/context/continuation_winfib.hpp>
#else
#include <uboost_coro/context/continuation_fcontext.hpp>
#endif
