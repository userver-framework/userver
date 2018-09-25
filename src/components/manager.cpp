#include <components/manager.hpp>

#include <future>
#include <stdexcept>

#include <components/component_list.hpp>
#include <engine/async.hpp>
#include <logging/component.hpp>
#include <logging/log.hpp>

#include <engine/task/task_context.hpp>
#include <engine/task/task_processor.hpp>

namespace {

const std::string kEventLoopThreadName = "event-loop";
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
  LOG_TRACE() << "Stopping event loops thread pool";
  event_thread_pool_.reset();
  LOG_TRACE() << "Stopped event loops thread pool";
  LOG_TRACE() << "Stopping coroutines pool";
  coro_pool_.reset();
  LOG_TRACE() << "Stopped coroutines pool";
  LOG_INFO() << "Stopped components manager";
}

const ManagerConfig& Manager::GetConfig() const { return config_; }

engine::TaskProcessor::CoroPool& Manager::GetCoroPool() const {
  return *coro_pool_;
}

engine::ev::ThreadPool& Manager::GetEventThreadPool() const {
  return *event_thread_pool_;
}

void Manager::OnLogRotate() {
  std::shared_lock<std::shared_timed_mutex> lock(context_mutex_);
  if (components_cleared_) return;
  auto* logger_component =
      component_context_->FindComponent<components::Logging>();
  if (logger_component) logger_component->OnLogRotate();
}

void Manager::AddComponents(const ComponentList& component_list) {
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
  LOG_INFO() << "All components loaded";
  component_context_->OnAllComponentsLoaded();
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

  try {
    LOG_TRACE() << "Adding component " << name;
    auto component = factory(config_it->second, *component_context_);
    component_context_->AddComponent(name, std::move(component));
    LOG_TRACE() << "Added component " << name;
  } catch (const std::exception& ex) {
    std::string message = "Cannot start component " + name + ": " + ex.what();
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
