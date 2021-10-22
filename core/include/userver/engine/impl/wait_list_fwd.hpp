#pragma once

#include <utility>  // for _LIBCPP_VERSION

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskContext;

class WaitList;
using FastPimplWaitList =
#ifdef _LIBCPP_VERSION
    utils::FastPimpl<WaitList, 88, 8>;
#else
    utils::FastPimpl<WaitList, 64, 8>;
#endif

class WaitListLight;
using FastPimplWaitListLight =
#ifndef NDEBUG
    utils::FastPimpl<WaitListLight, 24, 8>;
#else
    utils::FastPimpl<WaitListLight, 16, 8>;
#endif

}  // namespace engine::impl

USERVER_NAMESPACE_END
