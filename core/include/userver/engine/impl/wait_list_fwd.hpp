#pragma once

#include <userver/compiler/select.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskContext;

class WaitList;
constexpr inline std::size_t kWaitListSize = compiler::SelectSize()
                                                 .ForLibCpp64(88)
                                                 .ForLibStdCpp64(64)
                                                 .ForLibCpp32(88)
                                                 .ForLibStdCpp32(64);

using FastPimplWaitList =
    utils::FastPimpl<WaitList, kWaitListSize, alignof(void*)>;

class WaitListLight;
using FastPimplWaitListLight = utils::FastPimpl<WaitListLight, 16, 16>;

class GenericWaitList;
using FastPimplGenericWaitList = utils::FastPimpl<GenericWaitList, 112, 16>;

}  // namespace engine::impl

USERVER_NAMESPACE_END
