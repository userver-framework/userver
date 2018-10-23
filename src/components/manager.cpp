#include <components/manager.hpp>

#include <future>
#include <stdexcept>

#include <components/component_list.hpp>
#include <engine/async.hpp>
#include <logging/component.hpp>
#include <logging/log.hpp>

#include <engine/task/task_processor.hpp>
#include <engine/task/task_processor_pools.hpp>
#include "manager_config.hpp"

namespace {

const std::string kEngineMonitorDataName = "engine";

template <typename Func>
auto RunInCoro(engine::TaskProcessor& task_processor, const Func& func) {
  std::packaged_task<void()> task([&func] { func(); });
  auto future = task.get_future();
  engine::CriticalAsync(task_processor, std::move(task)).Detach();
  return future.get();
}

}  // namespace

namespace components {

Manager::Manager(std::unique_ptr<ManagerConfig>&& config,
                 const ComponentList& component_list)
    : config_(std::move(config)),
      components_cleared_(false),
      default_task_processor_(nullptr) {
  LOG_INFO() << "Starting components manager";

  task_processor_pools_ = std::make_shared<engine::impl::TaskProcessorPools>(
      config_->coro_pool, config_->event_thread_pool);

  components::ComponentContext::TaskProcessorMap task_processors;
  for (const auto& processor_config : config_->task_processors) {
    task_processors.emplace(processor_config.name,
                            std::make_unique<engine::TaskProcessor>(
                                processor_config, task_processor_pools_));
  }
  const auto default_task_processor_it =
      task_processors.find(config_->default_task_processor);
  if (default_task_processor_it == task_processors.end()) {
    throw std::runtime_error(
        "Cannot start components manager: missing default task processor");
  }
  default_task_processor_ = default_task_processor_it->second.get();
  component_context_ = std::make_unique<components::ComponentContext>(
      *this, std::move(task_processors));

  RunInCoro(*default_task_processor_,
            [this, &component_list]() { AddComponents(component_list); });

  LOG_INFO() << "Started components manager";
}

Manager::~Manager() {
  LOG_INFO() << "Stopping components manager";
  LOG_TRACE() << "Stopping component context";
  RunInCoro(*default_task_processor_, [this]() { ClearComponents(); });
  component_context_.reset();
  LOG_TRACE() << "Stopped component context";
  LOG_TRACE() << "Stopping task processor pools";
  assert(task_processor_pools_.use_count() == 1);
  task_processor_pools_.reset();
  LOG_TRACE() << "Stopped task processor_pools";
  LOG_INFO() << "Stopped components manager";
}

const ManagerConfig& Manager::GetConfig() const { return *config_; }

const std::shared_ptr<engine::impl::TaskProcessorPools>&
Manager::GetTaskProcessorPools() const {
  return task_processor_pools_;
}

void Manager::OnLogRotate() {
  std::shared_lock<std::shared_timed_mutex> lock(context_mutex_);
  if (components_cleared_) return;
  auto& logger_component =
      component_context_->FindComponent<components::Logging>();
  logger_component.OnLogRotate();
}

void Manager::AddComponents(const ComponentList& component_list) {
  components::ComponentConfigMap component_config_map;

  std::set<std::string> loading_component_names;
  for (const auto& component_config : config_->components) {
    const auto name = component_config.Name();
    loading_component_names.insert(name);
    component_config_map.emplace(name, component_config);
  }

  component_context_->SetLoadingComponentNames(loading_component_names);

  std::vector<engine::TaskWithResult<void>> tasks;
  try {
    for (const auto& adder : component_list) {
      tasks.push_back(engine::CriticalAsync([&]() {
        try {
          (*adder)(*this, component_config_map);
        } catch (...) {
          component_context_->CancelComponentsLoad();
          throw;
        }
      }));
    }

    for (auto& task : tasks) task.Get();
  } catch (const std::exception& ex) {
    component_context_->CancelComponentsLoad();

    /* Wait for all tasks to exit, but don't .Get() them - we've already caught
     * an exception, ignore the rest */
    for (auto& task : tasks) {
      task.Wait();
    }

    ClearComponents();
    throw;
  }
  LOG_INFO() << "All components loaded";
  component_context_->OnAllComponentsLoaded();
}  // namespace components

void Manager::AddComponentImpl(
    const components::ComponentConfigMap& config_map, const std::string& name,
    std::function<std::unique_ptr<components::ComponentBase>(
        const components::ComponentConfig&,
        const components::ComponentContext&)>
        factory) {
  LOG_INFO() << "Starting component " << name;
  const auto config_it = config_map.find(name);
  if (config_it == config_map.end()) {
    throw std::runtime_error("Cannot start component " + name +
                             ": missing config");
  }

  try {
    LOG_TRACE() << "Adding component " << name;
    component_context_->BeforeAddComponent(name);
    auto component = factory(config_it->second, *component_context_);
    component_context_->AddComponent(name, std::move(component));
    LOG_TRACE() << "Added component " << name;
  } catch (const std::exception& ex) {
    std::string message = "Cannot start component " + name + ": " + ex.what();
    component_context_->RemoveComponentDependencies(name);
    LOG_ERROR() << message;
    throw std::runtime_error(message);
  }
  LOG_INFO() << "Started component " << name;
}

void Manager::ClearComponents() {
  {
    std::unique_lock<std::shared_timed_mutex> lock(context_mutex_);
    components_cleared_ = true;
  }
  try {
    component_context_->ClearComponents();
  } catch (const std::exception& ex) {
    LOG_ERROR() << "error in clear components: " << ex.what();
  }
}

}  // namespace components
