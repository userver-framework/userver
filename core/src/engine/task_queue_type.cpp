#include <userver/engine/task_queue_type.hpp>

#include <userver/utils/trivial_map.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

TaskQueueType Parse(const yaml_config::YamlConfig& value, formats::parse::To<TaskQueueType>) {
    static constexpr utils::TrivialBiMap kMap([](auto selector) {
        return selector()
            .Case(TaskQueueType::kGlobalTaskQueue, "global-task-queue")
            .Case(TaskQueueType::kWorkStealingTaskQueue, "work-stealing-task-queue")
            .Case(TaskQueueType::kPullPinTaskQueue, "pull-pin-task-queue")
            // We do not allow selection from configs of kTSanTaskQueue
            // .Case(TaskQueueType::kTSanTaskQueue, "tsan-task-queue")
            ;
    });

    return utils::ParseFromValueString(value, kMap);
}

}  // namespace engine

USERVER_NAMESPACE_END
