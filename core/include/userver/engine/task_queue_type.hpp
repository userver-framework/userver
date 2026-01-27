#pragma once

/// @file userver/engine/task_queue_type.hpp
/// @brief @copybrief engine::TaskQueueType

#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @brief Scheduler types
enum class TaskQueueType {
    kGlobalTaskQueue,        /// < Global `moodycamel` queue from which each thread gets tasks
    kWorkStealingTaskQueue,  /// < Global+thread-specific queues with interqueues work stealing (experimental queue)
    kPullPinTaskQueue,  /// < Global+thread-specific queues. Each task gets pinned to a thread-specific queue and is
                        /// executed only in that thread (experimental queue)
    kTSanTaskQueue,  /// < Queue for TSan runs. Each task gets pinned to a thread-specific queue and is executed only in
                     /// that thread (experimental queue). Thread Sanitizer runs are automatically switched to this
                     /// queue
};

TaskQueueType Parse(const yaml_config::YamlConfig& value, formats::parse::To<TaskQueueType>);

}  // namespace engine

USERVER_NAMESPACE_END
