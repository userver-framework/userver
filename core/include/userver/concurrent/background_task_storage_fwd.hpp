#pragma once

/// @file userver/concurrent/background_task_storage_fwd.hpp
/// @brief Forward declarations for concurrent::BackgroundTaskStorage

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

class BackgroundTaskStorageCore;
class BackgroundTaskStorage;

using BackgroundTaskStorageFastPimpl = utils::FastPimpl<BackgroundTaskStorage, 208, 16>;

}  // namespace concurrent

USERVER_NAMESPACE_END
