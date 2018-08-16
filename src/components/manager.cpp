#include "manager.hpp"

#include <future>
#include <stdexcept>

#include <engine/async.hpp>
#include <engine/task/task_context.hpp>
#include <logging/component.hpp>
#include <logging/log.hpp>

#include "component_list.hpp"
#include "monitorable_component_base.hpp"

namespace {

const std::string kEventLoopThreadName = "event-loop";
const std::string kEngineMonitorDataName = "engine";

}  // namespace

namespace components {

Manager::Manager(ManagerConfig config, const ComponentList& component_list)
    : config_(std::move(config)),
      components_cleared_(false),
      default_task_processor_(nullptr) {
  LOG_INFO() << "Starting components manager";

  coro_pool_ = std::make_unique<engine::TaskProcessor::CoroPool>(
      config_.coro_pool, &engine::impl::TaskContext::CoroFunc);

  event_thread_pool_ = std::make_unique<engine::ev::ThreadPool>(
      config_.event_thread_pool.threads, kEventLoopThreadName);

  components::ComponentContext::TaskProcessorMap task_processors;
  for (const auto& processor_config : config_.task_processors) {
    task_processors.emplace(
        processor_config.name,
        std::make_unique<engine::TaskProcessor>(processor_config, *coro_pool_,
                                                *event_thread_pool_));
  }
  const auto default_task_processor_it =
      task_processors.find(config_.default_task_processor);
  if (default_task_processor_it == task_processors.end()) {
    throw std::runtime_error(
        "Cannot start components manager: missing default task processor");
  }
  default_task_processor_ = default_task_processor_it->second.get();
  component_context_ = std::make_unique<components::ComponentContext>(
      *this, std::move(task_processors));

  components::ComponentConfigMap component_config_map;
  for (const auto& component_config : config_.components) {
    component_config_map.emplace(component_config.Name(), component_config);
  }
  try {
    component_list.AddAll(*this, component_config_map);
  } catch (const std::exception& ex) {
    ClearComponents();
    throw;
  }

  LOG_INFO() << "Started components manager";
}

Manager::~Manager() {
  LOG_INFO() << "Stopping components manager";
  LOG_TRACE() << "Stopping component context";
  ClearComponents();
  component_context_.reset();
  LOG_TRACE() << "Stopped component context";
  LOG_TRACE() << "Stopping event loops thread pool";
  event_thread_pool_.reset();
  LOG_TRACE() << "Stopped event loops thread pool";
  LOG_TRACE() << "Stopping coroutines pool";
  coro_pool_.reset();
  LOG_TRACE() << "Stopped coroutines pool";
  LOG_INFO() << "Stopped components manager";
}

const ManagerConfig& Manager::GetConfig() const { return config_; }

engine::coro::PoolStats Manager::GetCoroutineStats() const {
  return coro_pool_->GetStats();
}

Json::Value Manager::GetMonitorData(MonitorVerbosity verbosity) const {
  Json::Value monitor_data(Json::objectValue);
  Json::Value engine_data(Json::objectValue);

  if (verbosity == MonitorVerbosity::kFull) {
    Json::Value json_task_processors(Json::arrayValue);
    for (const auto& task_processor : config_.task_processors) {
      Json::Value json_task_processor(Json::objectValue);
      json_task_processor["name"] = task_processor.name;
      json_task_processor["worker-threads"] =
          Json::UInt64{task_processor.worker_threads};
      json_task_processors[json_task_processors.size()] =
          std::move(json_task_processor);
    }
    engine_data["task-processors"] = std::move(json_task_processors);
  }

  auto coro_stats = coro_pool_->GetStats();
  {
    Json::Value json_coro_pool(Json::objectValue);

    Json::Value json_coro_stats(Json::objectValue);
    json_coro_stats["active"] = Json::UInt64{coro_stats.active_coroutines};
    json_coro_stats["total"] = Json::UInt64{coro_stats.total_coroutines};
    json_coro_pool["coroutines"] = std::move(json_coro_stats);

    engine_data["coro-pool"] = std::move(json_coro_pool);
  }
  monitor_data[kEngineMonitorDataName] = std::move(engine_data);

  {
    std::shared_lock<std::shared_timed_mutex> lock(context_mutex_);
    if (!components_cleared_) {
      for (const auto& component_item : *component_context_) {
        if (const auto* monitorable_component =
                dynamic_cast<MonitorableComponentBase*>(
                    component_item.second.get())) {
          if (component_item.first == kEngineMonitorDataName) {
            LOG_WARNING() << "skip monitor data from component named "
                          << component_item.first;
            continue;
          }
          monitor_data[component_item.first] =
              monitorable_component->GetMonitorData(verbosity);
        }
      }
    }
  }

  return monitor_data;
}

void Manager::OnLogRotate() {
  std::shared_lock<std::shared_timed_mutex> lock(context_mutex_);
  if (components_cleared_) return;
  auto* logger_component =
      component_context_->FindComponent<components::Logging>();
  if (logger_component) logger_component->OnLogRotate();
}

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

  std::packaged_task<void()> task([this, &name, &factory, config_it] {
    LOG_TRACE() << "Adding component " << name;
    component_context_->AddComponent(
        name, factory(config_it->second, *component_context_));
    LOG_TRACE() << "Added component " << name;
  });
  auto future = task.get_future();
  engine::CriticalAsync(*default_task_processor_, std::move(task)).Detach();
  try {
    future.get();
  } catch (const std::exception& ex) {
    throw std::runtime_error("Cannot start component " + name + ": " +
                             ex.what());
  }
  LOG_INFO() << "Started component " << name;
}

void Manager::ClearComponents() {
  {
    std::unique_lock<std::shared_timed_mutex> lock(context_mutex_);
    components_cleared_ = true;
  }
  std::packaged_task<void()> task(
      [this] { component_context_->ClearComponents(); });
  auto future = task.get_future();
  engine::CriticalAsync(*default_task_processor_, std::move(task)).Detach();
  try {
    future.get();
  } catch (const std::exception& ex) {
    LOG_ERROR() << "error in clear components: " << ex.what();
  }
}

}  // namespace components
