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
using FastPimplWaitListLight = utils::FastPimpl<WaitListLight, 8, 8>;

class GenericWaitList;
constexpr inline std::size_t kGenericWaitListSize = compiler::SelectSize()
                                                        .ForLibCpp64(96)
                                                        .ForLibStdCpp64(72)
                                                        .ForLibCpp32(96)
                                                        .ForLibStdCpp32(72);

using FastPimplGenericWaitList =
    utils::FastPimpl<GenericWaitList, kGenericWaitListSize, alignof(void*)>;

}  // namespace engine::impl

USERVER_NAMESPACE_END
