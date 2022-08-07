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
using FastPimplWaitListLight =
    utils::FastPimpl<WaitListLight, sizeof(void*) * 2, alignof(void*) * 2>;

class GenericWaitList;
constexpr inline std::size_t kGenericWaitListSize = compiler::SelectSize()
                                                        .ForLibCpp64(104)
                                                        .ForLibStdCpp64(80)
                                                        .ForLibCpp32(104)
                                                        .ForLibStdCpp32(80);

using FastPimplGenericWaitList =
    utils::FastPimpl<GenericWaitList, kGenericWaitListSize, alignof(void*) * 2>;

}  // namespace engine::impl

USERVER_NAMESPACE_END
