#pragma once

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

class BackgroundTaskStorage;

using BackgroundTaskStorageFastPimpl =
    utils::FastPimpl<BackgroundTaskStorage, 136, 8>;

}  // namespace concurrent

USERVER_NAMESPACE_END
