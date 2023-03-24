#include <userver/storages/secdist/secdist.hpp>

#include <cerrno>

#include <userver/compiler/demangle.hpp>
#include <userver/concurrent/async_event_channel.hpp>
#include <userver/engine/subprocess/environment_variables.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/secdist/exceptions.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/periodic_task.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::secdist {

namespace {

std::vector<std::function<std::any(const formats::json::Value&)>>&
GetConfigFactories() {
  static std::vector<std::function<std::any(const formats::json::Value&)>>
      factories;
  return factories;
}

}  // namespace

SecdistConfig::SecdistConfig() = default;

SecdistConfig::SecdistConfig(const SecdistConfig::Settings& settings) {
  // if we don't want to read secdist, then we don't need to initialize
  if (GetConfigFactories().empty()) return;

  Init(settings.provider->Get());
}

void SecdistConfig::Init(const formats::json::Value& doc) {
  for (const auto& config_factory : GetConfigFactories()) {
    configs_.emplace_back(config_factory(doc));
  }
}

std::size_t SecdistConfig::Register(
    std::function<std::any(const formats::json::Value&)>&& factory) {
  auto& config_factories = GetConfigFactories();
  config_factories.emplace_back(std::move(factory));
  return config_factories.size() - 1;
}

const std::any& SecdistConfig::Get(const std::type_index& type,
                                   std::size_t index) const {
  try {
    return configs_.at(index);
  } catch (const std::out_of_range&) {
    throw std::out_of_range("Type " + compiler::GetTypeName(type) +
                            " is not registered as config");
  }
}

class Secdist::Impl {
 public:
  explicit Impl(SecdistConfig::Settings settings);
  ~Impl();

  const storages::secdist::SecdistConfig& Get() const;

  rcu::ReadablePtr<storages::secdist::SecdistConfig> GetSnapshot() const;

  bool IsPeriodicUpdateEnabled() const;

  void EnsurePeriodicUpdateEnabled(const std::string& msg) const;

  concurrent::AsyncEventSubscriberScope DoUpdateAndListen(
      concurrent::FunctionId id, std::string_view name,
      EventSource::Function&& func);

 private:
  void StartUpdateTask();

  storages::secdist::SecdistConfig::Settings settings_;

  SecdistConfig secdist_config_;
  rcu::Variable<storages::secdist::SecdistConfig> dynamic_secdist_config_;
  concurrent::AsyncEventChannel<const SecdistConfig&> channel_;
  utils::PeriodicTask update_task_;
};

Secdist::Impl::Impl(SecdistConfig::Settings settings)
    : settings_(std::move(settings)),
      channel_("secdist", [this](auto& function) {
        const auto snapshot = dynamic_secdist_config_.Read();
        function(*snapshot);
      }) {
  dynamic_secdist_config_.Assign(storages::secdist::SecdistConfig(settings_));

  if (IsPeriodicUpdateEnabled()) {
    StartUpdateTask();
  }

  secdist_config_ = dynamic_secdist_config_.ReadCopy();
}

Secdist::Impl::~Impl() {
  if (IsPeriodicUpdateEnabled()) update_task_.Stop();
}

const SecdistConfig& Secdist::Impl::Get() const { return secdist_config_; }

rcu::ReadablePtr<SecdistConfig> Secdist::Impl::GetSnapshot() const {
  return dynamic_secdist_config_.Read();
}

bool Secdist::Impl::IsPeriodicUpdateEnabled() const {
  return settings_.update_period != std::chrono::milliseconds::zero();
}

concurrent::AsyncEventSubscriberScope Secdist::Impl::DoUpdateAndListen(
    concurrent::FunctionId id, std::string_view name,
    EventSource::Function&& func) {
  EnsurePeriodicUpdateEnabled(
      "Secdist update must be enabled to subscribe on it");
  auto func_copy = func;
  return channel_.DoUpdateAndListen(id, name, std::move(func), [&] {
    const auto snapshot = GetSnapshot();
    func_copy(*snapshot);
  });
}

void Secdist::Impl::EnsurePeriodicUpdateEnabled(const std::string& msg) const {
  if (!IsPeriodicUpdateEnabled()) {
    throw SecdistError(msg);
  }
}

void Secdist::Impl::StartUpdateTask() {
  LOG_INFO() << "Start task for secdist periodic updates";
  utils::PeriodicTask::Settings periodic_settings(
      settings_.update_period, {utils::PeriodicTask::Flags::kCritical});
  update_task_.Start("secdist_update", periodic_settings, [this]() {
    try {
      dynamic_secdist_config_.Assign(
          storages::secdist::SecdistConfig(settings_));
      auto snapshot = dynamic_secdist_config_.Read();
      channel_.SendEvent(*snapshot);
    } catch (const std::exception& ex) {
      LOG_ERROR() << "Secdist loading failed: " << ex;
    }
  });
}

Secdist::Secdist(SecdistConfig::Settings settings)
    : impl_(std::move(settings)) {}

Secdist::~Secdist() = default;

const SecdistConfig& Secdist::Get() const { return impl_->Get(); }

rcu::ReadablePtr<SecdistConfig> Secdist::GetSnapshot() const {
  return impl_->GetSnapshot();
}

bool Secdist::IsPeriodicUpdateEnabled() const {
  return impl_->IsPeriodicUpdateEnabled();
}

concurrent::AsyncEventSubscriberScope Secdist::DoUpdateAndListen(
    concurrent::FunctionId id, std::string_view name,
    EventSource::Function&& func) {
  return impl_->DoUpdateAndListen(id, name, std::move(func));
}

}  // namespace storages::secdist

USERVER_NAMESPACE_END
