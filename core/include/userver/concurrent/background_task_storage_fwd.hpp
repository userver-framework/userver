#pragma once

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

class BackgroundTaskStorageCore;
class BackgroundTaskStorage;

using BackgroundTaskStorageFastPimpl =
    utils::FastPimpl<BackgroundTaskStorage, 144, 16>;

}  // namespace concurrent

USERVER_NAMESPACE_END
