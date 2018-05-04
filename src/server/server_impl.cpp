#include "server_impl.hpp"

#include <future>
#include <stdexcept>

#include <components/thread_pool.hpp>
#include <engine/async.hpp>
#include <engine/task/task.hpp>
#include <logging/component.hpp>
#include <logging/log.hpp>

#include "component_list.hpp"
#include "server_monitor.hpp"

namespace server {

ServerImpl::ServerImpl(ServerConfig config, const ComponentList& component_list)
    : config_(std::move(config)), default_task_processor_(nullptr) {
  LOG_INFO() << "Starting server";

  coro_pool_ = std::make_unique<engine::TaskProcessor::CoroPool>(
      config_.coro_pool, &engine::Task::CoroFunc);

  for (const auto& event_thread_pool_config : config_.event_thread_pools) {
    event_thread_pools_.emplace(std::piecewise_construct,
                                std::tie(event_thread_pool_config.name),
                                std::tie(event_thread_pool_config.threads,
                                         event_thread_pool_config.thread_name));
  }

  components::ComponentContext::TaskProcessorMap task_processors;
  for (const auto& processor_config : config_.task_processors) {
    task_processors.emplace(
        processor_config.name,
        std::make_unique<engine::TaskProcessor>(
            processor_config, *coro_pool_,
            event_thread_pools_.at(processor_config.event_thread_pool)));
  }
  const auto default_task_processor_it =
      task_processors.find(config_.default_task_processor);
  if (default_task_processor_it == task_processors.end()) {
    throw std::runtime_error(
        "Cannot start server: missing default task processor");
  }
  default_task_processor_ = default_task_processor_it->second.get();
  auto monitor = std::make_unique<ServerMonitor>(*this);
  component_context_ = std::make_unique<components::ComponentContext>(
      std::move(task_processors), std::move(monitor));

  components::ComponentConfigMap component_config_map;
  for (const auto& component_config : config_.components) {
    component_config_map.emplace(component_config.Name(), component_config);
  }
  component_list.AddAll(*this, component_config_map);

  request_handler_ = std::make_unique<request_handling::RequestHandler>(
      *component_context_, config_.logger_access, config_.logger_access_tskv);

  endpoint_info_ =
      std::make_shared<net::EndpointInfo>(config_.listener, *request_handler_);

  auto& event_thread_pool = default_task_processor_->EventThreadPool();
  auto event_thread_controls =
      event_thread_pool.NextThreads(event_thread_pool.size());
  for (auto& event_thread_control : event_thread_controls) {
    listeners_.emplace_back(endpoint_info_, *default_task_processor_,
                            *event_thread_control);
  }

  LOG_INFO() << "Started server";
}

ServerImpl::~ServerImpl() {
  LOG_INFO() << "Stopping server";
  LOG_TRACE() << "Stopping listeners";
  listeners_.clear();
  LOG_TRACE() << "Stopped listeners";
  LOG_TRACE() << "Stopping request handler";
  request_handler_.reset();
  LOG_TRACE() << "Stopped request handler";
  LOG_TRACE() << "Stopping component context";
  component_context_.reset();
  LOG_TRACE() << "Stopped component context";
  LOG_TRACE() << "Stopping event_thread_pools";
  event_thread_pools_.clear();
  LOG_TRACE() << "Stopped event_thread_pools";
  LOG_TRACE() << "Stopping coroutines pool";
  coro_pool_.reset();
  LOG_TRACE() << "Stopped coroutines pool";
  LOG_INFO() << "Stopped server";
}

const ServerConfig& ServerImpl::GetConfig() const { return config_; }

net::Stats ServerImpl::GetNetworkStats() const {
  net::Stats summary;
  for (const auto& listener : listeners_) {
    summary += listener.GetStats();
  }
  return summary;
}

engine::coro::PoolStats ServerImpl::GetCoroutineStats() const {
  return coro_pool_->GetStats();
}

void ServerImpl::OnLogRotate() {
  auto* logger_component =
      component_context_->FindComponent<components::Logging>();
  if (logger_component) logger_component->OnLogRotate();
}

void ServerImpl::AddComponentImpl(
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
  engine::Async(*default_task_processor_, std::move(task));
  try {
    future.get();
  } catch (const std::exception& ex) {
    throw std::runtime_error("Cannot start component " + name + ": " +
                             ex.what());
  }
  LOG_INFO() << "Started component " << name;
}

}  // namespace server
