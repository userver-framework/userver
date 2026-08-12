#pragma once

#include <engine/impl/standalone.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::tests {

class TwoStandaloneTaskProcessors final {
public:
    TwoStandaloneTaskProcessors()
        : pools_(engine::impl::MakeTaskProcessorPools({})),
          main_task_processor_(
              engine::impl::TaskProcessorHolder::Make(1, "main", engine::TaskQueueType::kGlobalTaskQueue, pools_)
          ),
          secondary_task_processor_(
              engine::impl::TaskProcessorHolder::Make(1, "secondary", engine::TaskQueueType::kGlobalTaskQueue, pools_)
          )
    {}

    auto& GetMain() { return *main_task_processor_; }
    auto& GetSecondary() { return *secondary_task_processor_; }

    template <typename Func>
    void RunBlocking(Func&& func) {
        engine::impl::RunOnTaskProcessorSync(GetMain(), std::forward<Func>(func));
    }

private:
    std::shared_ptr<engine::impl::TaskProcessorPools> pools_;
    engine::impl::TaskProcessorHolder main_task_processor_;
    engine::impl::TaskProcessorHolder secondary_task_processor_;
};

}  // namespace engine::tests

USERVER_NAMESPACE_END
