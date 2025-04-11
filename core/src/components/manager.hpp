#pragma once

#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <userver/components/component_context.hpp>
#include <userver/components/component_fwd.hpp>
#include <userver/components/component_list.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/impl/transparent_hash.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {
class TaskProcessorPools;
}  // namespace engine::impl

namespace os_signals {
class ProcessorComponent;
}  // namespace os_signals

namespace components {
class ComponentList;
struct ManagerConfig;
}  // namespace components

namespace components::impl {

class ComponentAdderBase;

using ComponentConfigMap = std::unordered_map<std::string, const ComponentConfig&>;
using TaskProcessorsMap = utils::impl::TransparentMap<std::string, std::unique_ptr<engine::TaskProcessor>>;

// Responsible for instantiating coroutine engine, wrapping ComponentContextImpl in it and bridging it
// to the outside world.
class Manager final {
public:
    Manager(std::unique_ptr<ManagerConfig>&& config, const ComponentList& component_list);
    ~Manager();

    const ManagerConfig& GetConfig() const;
    const std::shared_ptr<engine::impl::TaskProcessorPools>& GetTaskProcessorPools() const;
    const TaskProcessorsMap& GetTaskProcessorsMap() const;
    engine::TaskProcessor& GetTaskProcessor(std::string_view name) const;

    void OnSignal(int signum);

    std::chrono::steady_clock::time_point GetStartTime() const;

    std::chrono::milliseconds GetLoadDuration() const;

private:
    class TaskProcessorsStorage final {
    public:
        explicit TaskProcessorsStorage(std::shared_ptr<engine::impl::TaskProcessorPools> task_processor_pools);
        ~TaskProcessorsStorage();

        void Reset() noexcept;

        const TaskProcessorsMap& GetMap() const { return task_processors_map_; }

        const std::shared_ptr<engine::impl::TaskProcessorPools>& GetTaskProcessorPools() const {
            return task_processor_pools_;
        }

        void Add(std::string name, std::unique_ptr<engine::TaskProcessor>&& task_processor);

    private:
        void WaitForAllTasksBlocking() const noexcept;

        std::shared_ptr<engine::impl::TaskProcessorPools> task_processor_pools_;
        TaskProcessorsMap task_processors_map_;
    };

    void CreateComponentContext(const ComponentList& component_list);
    void AddComponents(const ComponentList& component_list);

    void AddComponentImpl(
        const components::ComponentConfigMap& config_map,
        const std::string& name,
        const impl::ComponentAdderBase& adder
    );
    void ClearComponents() noexcept;
    components::ComponentConfigMap MakeComponentConfigMap(const ComponentList& component_list);

    std::unique_ptr<const ManagerConfig> config_;
    std::vector<ComponentConfig> empty_configs_;
    TaskProcessorsStorage task_processors_storage_;

    mutable std::shared_timed_mutex context_mutex_;
    std::unique_ptr<impl::ComponentContextImpl> component_context_;
    bool components_cleared_{false};

    engine::TaskProcessor* default_task_processor_{nullptr};
    const std::chrono::steady_clock::time_point start_time_;
    std::chrono::milliseconds load_duration_{0};

    os_signals::ProcessorComponent* signal_processor_{nullptr};
};

}  // namespace components::impl

USERVER_NAMESPACE_END
