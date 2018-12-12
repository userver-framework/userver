#include <components/manager.hpp>

#include <future>
#include <stdexcept>
#include <type_traits>

#include <components/component_list.hpp>
#include <engine/async.hpp>
#include <logging/log.hpp>
#include <utils/async.hpp>

#include <engine/task/task_processor.hpp>
#include <engine/task/task_processor_pools.hpp>
#include "manager_config.hpp"

namespace {

const std::string kEngineMonitorDataName = "engine";

template <typename Func>
auto RunInCoro(engine::TaskProcessor& task_processor, Func&& func) {
  if (auto* task_context =
          engine::current_task::GetCurrentTaskContextUnchecked()) {
    if (&task_processor == &task_context->GetTaskProcessor())
      return func();
    else
      return engine::CriticalAsync(task_processor, std::forward<Func>(func))
          .Get();
  }
  std::packaged_task<std::result_of_t<Func()>()> task(
      [&func] { return func(); });
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
      default_task_processor_(nullptr),
      start_time_(std::chrono::steady_clock::now()) {
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
  RunInCoro(*default_task_processor_, [
    this, task_processors = std::move(task_processors), &component_list
  ]() mutable {
    CreateComponentContext(std::move(task_processors), component_list);
  });

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
  if (logging_component_) logging_component_->OnLogRotate();
}

std::chrono::steady_clock::time_point Manager::GetStartTime() const {
  return start_time_;
}

std::chrono::milliseconds Manager::GetLoadDuration() const {
  return load_duration_;
}

void Manager::CreateComponentContext(
    components::ComponentContext::TaskProcessorMap&& task_processors,
    const ComponentList& component_list) {
  std::set<std::string> loading_component_names;
  for (const auto& adder : component_list) {
    auto[it, inserted] =
        loading_component_names.insert(adder->GetComponentName());
    if (!inserted) {
      std::string message =
          "duplicate component name in component_list: " + *it;
      LOG_ERROR() << message;
      throw std::runtime_error(message);
    }
  }
  component_context_ = std::make_unique<components::ComponentContext>(
      *this, std::move(task_processors), loading_component_names);

  AddComponents(component_list);
}

void Manager::AddComponents(const ComponentList& component_list) {
  components::ComponentConfigMap component_config_map;

  for (const auto& component_config : config_->components) {
    const auto name = component_config.Name();
    component_config_map.emplace(name, component_config);
  }

  auto start_time = std::chrono::steady_clock::now();
  std::vector<engine::TaskWithResult<void>> tasks;
  bool is_load_cancelled = false;
  try {
    for (const auto& adder : component_list) {
      auto task_name = "start_" + adder->GetComponentName();
      tasks.push_back(utils::CriticalAsync(task_name, [&]() {
        try {
          (*adder)(*this, component_config_map);
        } catch (const ComponentsLoadCancelledException& ex) {
          LOG_WARNING() << "Cannot start component "
                        << adder->GetComponentName() << ": " << ex.what();
          component_context_->CancelComponentsLoad();
          throw;
        } catch (const std::exception& ex) {
          LOG_ERROR() << "Cannot start component " << adder->GetComponentName()
                      << ": " << ex.what();
          component_context_->CancelComponentsLoad();
          throw;
        } catch (...) {
          component_context_->CancelComponentsLoad();
          throw;
        }
      }));
    }

    for (auto& task : tasks) {
      try {
        task.Get();
      } catch (const ComponentsLoadCancelledException&) {
        is_load_cancelled = true;
      }
    }
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

  if (is_load_cancelled) {
    ClearComponents();
    throw std::logic_error(
        "Components load cancelled, but only ComponentsLoadCancelledExceptions "
        "were caught");
  }

  LOG_INFO() << "All components created";
  try {
    component_context_->OnAllComponentsLoaded();
  } catch (const std::exception& ex) {
    ClearComponents();
    throw;
  }

  auto stop_time = std::chrono::steady_clock::now();
  load_duration_ = std::chrono::duration_cast<std::chrono::milliseconds>(
      stop_time - start_time);

  LOG_INFO() << "All components loaded";
}

void Manager::AddComponentImpl(
    const components::ComponentConfigMap& config_map, const std::string& name,
    std::function<std::unique_ptr<components::ComponentBase>(
        const components::ComponentConfig&,
        const components::ComponentContext&)>
        factory) {
  const auto config_it = config_map.find(name);
  if (config_it == config_map.end()) {
    throw std::runtime_error("Cannot start component " + name +
                             ": missing config");
  }
  auto enabled =
      config_it->second.ParseOptionalBool("load-enabled").value_or(true);
  if (!enabled) {
    LOG_INFO() << "Component " << name
               << " load disabled in config.yaml, skipping";
    return;
  }

  LOG_INFO() << "Starting component " << name;

  auto* component = component_context_->AddComponent(name, [
    &factory, &config = config_it->second
  ](const components::ComponentContext& component_context) {
    return factory(config, component_context);
  });
  if (auto* logging_component = dynamic_cast<components::Logging*>(component))
    logging_component_ = logging_component;

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
