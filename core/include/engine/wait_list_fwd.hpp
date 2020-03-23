#pragma once

#include <utility>  // for _LIBCPP_VERSION

#include <utils/fast_pimpl.hpp>

namespace engine::impl {

class TaskContext;

class WaitList;
using FastPimplWaitList =
#ifdef _LIBCPP_VERSION
    ::utils::FastPimpl<WaitList, 96, 8>;
#else
    ::utils::FastPimpl<WaitList, 72, 8>;
#endif

class WaitListLight;
using FastPimplWaitListLight =
#ifndef NDEBUG
    ::utils::FastPimpl<WaitListLight, 32, 8>;
#else
    ::utils::FastPimpl<WaitListLight, 24, 8>;
#endif

}  // namespace engine::impl
