#pragma once

#include <userver/compiler/select.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskContext;

class WaitList;
using FastPimplWaitList = utils::FastPimpl<WaitList, 32, 8>;

class WaitListLight;
using FastPimplWaitListLight = utils::FastPimpl<WaitListLight, 16, 16>;

class GenericWaitList;
using FastPimplGenericWaitList = utils::FastPimpl<GenericWaitList, 48, 16>;

}  // namespace engine::impl

USERVER_NAMESPACE_END
