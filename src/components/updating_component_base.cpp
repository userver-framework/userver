#include <components/updating_component_base.hpp>

#include <cstdint>

#include <engine/async.hpp>
#include <logging/log.hpp>

namespace components {

namespace {

const uint64_t kDefaultUpdateIntervalMs = 5 * 60 * 1000;

uint64_t GetDefaultJitterMs(const std::chrono::milliseconds& interval) {
  return (interval / 10).count();
}

}  // namespace

const std::string& UpdatingComponentBase::Name() const { return name_; }

UpdatingComponentBase::UpdatingComponentBase(const ComponentConfig& config,
                                             std::string name)
    : name_(std::move(name)),
      update_interval_(
          config.ParseUint64("update_interval_ms", kDefaultUpdateIntervalMs)),
      update_jitter_(config.ParseUint64("update_jitter_ms",
                                        GetDefaultJitterMs(update_interval_))),
      full_update_interval_(config.ParseUint64("full_update_interval_ms", 0)),
      is_running_(false),
      jitter_generator_(std::random_device()()) {}

UpdatingComponentBase::~UpdatingComponentBase() {
  if (is_running_.load()) {
    LOG_ERROR()
        << "UpdatingComponentBase is being destroyed while periodic update "
           "task is still running. "
           "Derived class has to call StopPeriodicUpdates() in destructor. "
        << "Component name '" << name_ << "'";
    // Don't crash in production
    assert(false && "StopPeriodicUpdates() is not called");
  }
}

void UpdatingComponentBase::StartPeriodicUpdates() {
  if (is_running_.exchange(true)) {
    return;
  }

  // Force first update
  engine::CriticalAsync([this] { DoPeriodicUpdate(); }).Get();

  auto task = [this] {
    while (is_running_) {
      std::uniform_int_distribution<decltype(update_jitter_)::rep>
          jitter_distribution(-update_jitter_.count(), update_jitter_.count());
      decltype(update_jitter_) jitter(jitter_distribution(jitter_generator_));

      {
        std::unique_lock<engine::Mutex> lock(is_running_mutex_);
        if (is_running_cv_.WaitFor(lock, update_interval_ + jitter,
                                   [this] { return !is_running_; })) {
          break;
        }
      }

      try {
        DoPeriodicUpdate();
      } catch (const std::exception& ex) {
        LOG_WARNING() << "Cannot update " << name_ << ": " << ex.what();
      }
    }
  };
  update_task_ = engine::CriticalAsync(std::move(task));
}

void UpdatingComponentBase::StopPeriodicUpdates() {
  {
    std::lock_guard<engine::Mutex> lock(is_running_mutex_);
    if (!is_running_.exchange(false) || !update_task_) {
      return;
    }
  }
  is_running_cv_.NotifyAll();
  try {
    update_task_.Get();
  } catch (const std::exception& ex) {
    LOG_ERROR() << "exception in update task: " << ex.what();
  }
}

void UpdatingComponentBase::DoPeriodicUpdate() {
  const auto steady_now = std::chrono::steady_clock::now();
  auto update_type = UpdateType::kFull;
  if (last_full_update_ + full_update_interval_ > steady_now) {
    update_type = UpdateType::kIncremental;
  }

  const auto system_now = std::chrono::system_clock::now();
  Update(update_type, last_update_, system_now);

  last_update_ = system_now;
  if (update_type == UpdateType::kFull) {
    last_full_update_ = steady_now;
  }
}

}  // namespace components
