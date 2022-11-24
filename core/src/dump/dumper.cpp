#include <userver/dump/dumper.hpp>

#include <fmt/format.h>
#include <boost/filesystem/operations.hpp>

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/tracing/scope_time.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/atomic.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/statistics/storage.hpp>

#include <dump/dump_locator.hpp>
#include <dump/statistics.hpp>
#include <userver/components/dump_configurator.hpp>
#include <userver/dump/config.hpp>
#include <userver/dump/factory.hpp>
#include <userver/testsuite/dump_control.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

void ThrowDumpUnimplemented(const std::string& name) {
  UINVARIANT(false, fmt::format("Dumps are unimplemented for {}. "
                                "See dump::Read, dump::Write",
                                name));
}

DumpableEntity::~DumpableEntity() = default;

namespace {

struct UpdateTime final {
  TimePoint last_update;
  TimePoint last_modifying_update;
};

struct DumpData {
  DumpData(const Config& static_config,
           std::unique_ptr<OperationsFactory> rw_factory,
           DumpableEntity& dumpable)
      : rw_factory(std::move(rw_factory)),
        dumpable(dumpable),
        locator(static_config) {
    UASSERT(this->rw_factory);
  }

  const std::unique_ptr<OperationsFactory> rw_factory;
  DumpableEntity& dumpable;
  DumpLocator locator;
  std::optional<UpdateTime> dumped_update_time;
};

struct UpdateData {
  explicit UpdateData(Statistics& statistics)
      : is_current_from_dump(statistics.is_current_from_dump) {}

  std::optional<UpdateTime> update_time;
  std::atomic<bool>& is_current_from_dump;
};

Config ParseConfig(const components::ComponentConfig& config,
                   const components::ComponentContext& context) {
  return Config{
      config.Name(), config[kDump],
      context.FindComponent<components::DumpConfigurator>().GetDumpRoot()};
}

using NoAutoReset = engine::SingleConsumerEvent::NoAutoReset;

enum class UpdateSignalingStatus {
  kNotSignaled,
  kSignaling,
  kSignaled,
};

engine::Deadline GetCooldownDeadline(
    const DynamicConfig& config,
    engine::Deadline::TimePoint previous_write_time) {
  if (!config.dumps_enabled) return {};
  return engine::Deadline::FromTimePoint(previous_write_time +
                                         config.min_dump_interval);
}

}  // namespace

class Dumper::Impl {
 public:
  Impl(const Config& initial_config,
       std::unique_ptr<OperationsFactory> rw_factory,
       engine::TaskProcessor& fs_task_processor,
       dynamic_config::Source config_source,
       utils::statistics::Storage& statistics_storage,
       testsuite::DumpControl& dump_control, DumpableEntity& dumpable,
       Dumper& self);

  ~Impl();

  const std::string& Name() const;

  std::optional<TimePoint> ReadDump();

  void WriteDumpSyncDebug();

  void ReadDumpDebug();

  void OnUpdateCompleted();

  void OnUpdateCompleted(TimePoint update_time, UpdateType update_type);

  void CancelWriteTaskAndWait() noexcept;

 private:
  void PeriodicWriteTask();

  /// @throws std::exception on failure
  void WriteDump(DumpData& dump_data);

  UpdateTime RetrieveUpdateTime(UpdateData& update_data);

  /// @throws std::exception on failure
  void DoWriteDump(TimePoint update_time, tracing::ScopeTime& scope,
                   DumpData& dump_data);

  enum class DumpOperation { kNewDump, kBumpTime };

  /// @returns `update_time` of the loaded dump on success, `null` otherwise
  std::optional<TimePoint> LoadFromDump(DumpData& dump_data,
                                        const DynamicConfig& config);

  rcu::ReadablePtr<DynamicConfig> ReadConfigForPeriodicTask();

  void OnConfigUpdate(const dynamic_config::Snapshot& config);

  formats::json::Value ExtendStatistics() const;

  const Config static_config_;
  const std::string write_span_name_;
  const std::string read_span_name_;
  rcu::Variable<DynamicConfig> dynamic_config_;
  engine::TaskProcessor& fs_task_processor_;
  Statistics statistics_;

  engine::SingleConsumerEvent config_updated_signal_{NoAutoReset{}};

  // Fields in this group must only be modified under the 'update_data_' lock.
  engine::SingleConsumerEvent data_updated_signal_{NoAutoReset{}};
  // Used by the OnUpdateCompleted overload without args.
  std::atomic<UpdateSignalingStatus> update_signaling_status_{
      UpdateSignalingStatus::kNotSignaled};

  concurrent::Variable<DumpData> dump_data_;
  concurrent::Variable<UpdateData> update_data_;

  engine::TaskWithResult<void> periodic_task_;

  utils::statistics::Entry statistics_holder_;
  concurrent::AsyncEventSubscriberScope config_subscription_;
  std::optional<testsuite::DumperRegistrationHolder> testsuite_registration_;
};

Dumper::Impl::Impl(const Config& initial_config,
                   std::unique_ptr<OperationsFactory> rw_factory,
                   engine::TaskProcessor& fs_task_processor,
                   dynamic_config::Source config_source,
                   utils::statistics::Storage& statistics_storage,
                   testsuite::DumpControl& dump_control,
                   DumpableEntity& dumpable, Dumper& self)
    : static_config_(initial_config),
      write_span_name_("write-dump/" + Name()),
      read_span_name_("read-dump/" + Name()),
      dynamic_config_(static_config_, ConfigPatch{}),
      fs_task_processor_(fs_task_processor),
      dump_data_(static_config_, std::move(rw_factory), dumpable),
      update_data_(statistics_),
      testsuite_registration_(std::in_place, dump_control, self) {
  statistics_holder_ = statistics_storage.RegisterExtender(
      fmt::format("cache.{}.dump", Name()),
      [this](auto&) { return ExtendStatistics(); });
  config_subscription_ = config_source.UpdateAndListen(this, "dump." + Name(),
                                                       &Impl::OnConfigUpdate);
  periodic_task_ =
      engine::AsyncNoSpan(fs_task_processor_, [this] { PeriodicWriteTask(); });
}

Dumper::Impl::~Impl() {
  if (periodic_task_.IsValid()) {
    LOG_INFO() << Name()
               << ": stopping the periodic Dumper task automatically. "
                  "Please call CancelWriteTaskAndWait manually in destructors "
                  "to avoid UB at shutdown.";
  }
  CancelWriteTaskAndWait();
  config_subscription_.Unsubscribe();
  statistics_holder_.Unregister();
}

const std::string& Dumper::Impl::Name() const { return static_config_.name; }

std::optional<TimePoint> Dumper::Impl::ReadDump() {
  auto dump_data = dump_data_.Lock();
  const auto config = dynamic_config_.Read();

  return LoadFromDump(*dump_data, *config);
}

void Dumper::Impl::WriteDumpSyncDebug() {
  const auto config = dynamic_config_.Read();
  if (!config->dumps_enabled) {
    throw Error(fmt::format("{}: not ready to write a dump, dumps are disabled",
                            Name()));
  }

  engine::AsyncNoSpan(fs_task_processor_, [&] {
    auto dump_data = dump_data_.Lock();
    tracing::Span span(write_span_name_);
    WriteDump(*dump_data);
  }).Get();
}

void Dumper::Impl::ReadDumpDebug() {
  auto dump_data = dump_data_.Lock();
  const auto config = dynamic_config_.Read();

  const auto update_time = LoadFromDump(*dump_data, *config);

  if (!update_time) {
    throw Error(fmt::format(
        "{}: failed to read a dump when explicitly requested", Name()));
  }
}

void Dumper::Impl::OnUpdateCompleted() {
  auto expected = UpdateSignalingStatus::kNotSignaled;
  if (update_signaling_status_.load() != expected ||
      !update_signaling_status_.compare_exchange_strong(
          expected, UpdateSignalingStatus::kSignaling)) {
    // If kSignaled, then our job is already done.
    // If kSignaling, then let's pretend we are sequenced right before them.
    return;
  }

  // In case of multiple concurrent updaters, only one updater per a dump write
  // will end up potentially waiting on the mutex. The vast majority of updates
  // will be wait-free.
  auto update_data = update_data_.Lock();
  update_signaling_status_.store(UpdateSignalingStatus::kSignaled,
                                 std::memory_order_relaxed);
  data_updated_signal_.Send();
}

void Dumper::Impl::OnUpdateCompleted(TimePoint update_time,
                                     UpdateType update_type) {
  auto update_data = update_data_.Lock();

  if (update_type == UpdateType::kModified) {
    update_data->update_time = {update_time, update_time};
    update_data->is_current_from_dump = false;
  } else if (update_data->update_time) {
    update_data->update_time->last_update = update_time;
  } else {
    LOG_WARNING() << Name() << ": no successful updates have been performed";
  }

  data_updated_signal_.Send();
}

void Dumper::Impl::PeriodicWriteTask() {
  bool previous_write_succeeded = true;
  auto previous_write_time = engine::Deadline::TimePoint::min();

  while (true) {
    const auto config = ReadConfigForPeriodicTask();

    if (previous_write_succeeded && !data_updated_signal_.IsReady()) {
      if (!data_updated_signal_.WaitForEvent()) break;
      if (config_updated_signal_.IsReady()) {
        continue;  // reload config
      }
    }

    const auto cooldown = GetCooldownDeadline(*config, previous_write_time);
    if (!cooldown.IsReached()) {
      if (config_updated_signal_.WaitForEventUntil(cooldown)) {
        continue;  // reload config
      }
      if (engine::current_task::ShouldCancel()) break;
    }

    auto dump_data = dump_data_.Lock();
    tracing::Span span(write_span_name_);

    try {
      WriteDump(*dump_data);
    } catch (const std::exception& ex) {
      LOG_ERROR() << Name() << ": failed to write a dump. Reason: " << ex;
    }

    previous_write_time = engine::Deadline::Clock::now();
  }
}

void Dumper::Impl::WriteDump(DumpData& dump_data) {
  auto scope_time = tracing::Span::CurrentSpan().CreateScopeTime();
  LOG_DEBUG() << Name() << ": requested to write a dump";

  const auto update_time = [&] {
    auto update_data = update_data_.Lock();
    return RetrieveUpdateTime(*update_data);
  }();

  const auto& dumped_update_time = dump_data.dumped_update_time;

  auto operation_type = DumpOperation::kNewDump;
  if (dumped_update_time && dumped_update_time->last_modifying_update ==
                                update_time.last_modifying_update) {
    LOG_DEBUG() << Name() << ": skipped dump, because nothing has been updated";
    operation_type = DumpOperation::kBumpTime;
  }

  switch (operation_type) {
    case DumpOperation::kNewDump: {
      dump_data.locator.Cleanup();
      DoWriteDump(update_time.last_update, scope_time, dump_data);
      break;
    }
    case DumpOperation::kBumpTime: {
      UASSERT(dumped_update_time);
      if (!dump_data.locator.BumpDumpTime(dumped_update_time->last_update,
                                          update_time.last_update)) {
        DoWriteDump(update_time.last_update, scope_time, dump_data);
      }
      break;
    }
  }

  dump_data.dumped_update_time = update_time;
}

UpdateTime Dumper::Impl::RetrieveUpdateTime(UpdateData& update_data) {
  data_updated_signal_.Reset();

  // kSignaling updates will wait for the next write iteration
  // (after we release UpdateData) to finish signaling.
  if (update_signaling_status_.load() == UpdateSignalingStatus::kSignaled) {
    update_signaling_status_.store(UpdateSignalingStatus::kNotSignaled);
    const auto now = std::chrono::time_point_cast<TimePoint::duration>(
        utils::datetime::Now());
    update_data.update_time = {now, now};
  }

  if (update_data.update_time) {
    return *update_data.update_time;
  }

  throw Error(
      fmt::format("{}: not ready to write a dump, OnUpdateCompleted has "
                  "never been called",
                  Name()));
}

rcu::ReadablePtr<DynamicConfig> Dumper::Impl::ReadConfigForPeriodicTask() {
  // A race condition could potentially occur in the following sequence:
  // 1. thread1: config_.Assign(...)
  // 2. thread1: config_updated_.Send()
  // 3. thread2: config_updated_.Reset()
  // 4. thread2: somehow reads old config in config_.Read()
  // 5. thread2: never knows about the config update
  // seq_cst is required to prevent this from happening.
  config_updated_signal_.Reset();
  std::atomic_thread_fence(std::memory_order_seq_cst);
  return dynamic_config_.Read();
}

void Dumper::Impl::OnConfigUpdate(const dynamic_config::Snapshot& config) {
  const auto patch =
      utils::FindOptional(config[kConfigSet], Name()).value_or(ConfigPatch{});
  DynamicConfig new_config{static_config_, patch};
  const auto old_config = dynamic_config_.Read();

  dynamic_config_.Assign(std::move(new_config));

  if (new_config.min_dump_interval != old_config->min_dump_interval ||
      new_config.dumps_enabled != old_config->dumps_enabled) {
    // See the comment in ReadConfigForPeriodicTask
    std::atomic_thread_fence(std::memory_order_seq_cst);
    config_updated_signal_.Send();
  }
}

formats::json::Value Dumper::Impl::ExtendStatistics() const {
  return formats::json::ValueBuilder{statistics_}.ExtractValue();
}

void Dumper::Impl::CancelWriteTaskAndWait() noexcept {
  testsuite_registration_.reset();
  periodic_task_.SyncCancel();
}

void Dumper::Impl::DoWriteDump(TimePoint update_time, tracing::ScopeTime& scope,
                               DumpData& dump_data) {
  const auto dump_start = std::chrono::steady_clock::now();

  const auto dump_stats = dump_data.locator.RegisterNewDump(update_time);
  const auto& dump_path = dump_stats.full_path;
  auto writer = dump_data.rw_factory->CreateWriter(dump_path, scope);
  dump_data.dumpable.GetAndWrite(*writer);
  writer->Finish();
  const auto dump_size = boost::filesystem::file_size(dump_path);

  LOG_INFO() << Name() << ": a new dump has been written at \"" << dump_path
             << '"';

  statistics_.last_written_size = dump_size;
  statistics_.last_nontrivial_write_duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - dump_start);
  statistics_.last_nontrivial_write_start_time = dump_start;
}

std::optional<TimePoint> Dumper::Impl::LoadFromDump(
    DumpData& dump_data, const DynamicConfig& config) {
  if (!config.dumps_enabled) {
    LOG_DEBUG() << Name()
                << ": could not load a dump, because dumps are disabled for "
                   "this dumper";
    return {};
  }

  const auto load_start = std::chrono::steady_clock::now();

  const std::optional<TimePoint> update_time =
      utils::Async(fs_task_processor_, read_span_name_, [&] {
        auto scope_time = tracing::Span::CurrentSpan().CreateScopeTime();

        try {
          auto dump_stats = dump_data.locator.GetLatestDump();
          if (!dump_stats) return std::optional<TimePoint>{};

          auto reader =
              dump_data.rw_factory->CreateReader(dump_stats->full_path);
          dump_data.dumpable.ReadAndSet(*reader);
          reader->Finish();

          LOG_INFO() << Name() << ": a dump has been loaded successfully";
          return std::optional{dump_stats->update_time};
        } catch (const std::exception& ex) {
          LOG_ERROR() << Name()
                      << ": error while reading a dump. Reason: " << ex;
          return std::optional<TimePoint>{};
        }
      }).Get();

  if (!update_time) return {};
  const UpdateTime update_times{*update_time, *update_time};

  {
    auto update_data = update_data_.Lock();
    update_data->update_time = update_times;
    update_data->is_current_from_dump = true;
  }
  // So that we don't attempt to write the dump we've just read
  dump_data.dumped_update_time = update_times;

  statistics_.is_loaded = true;
  statistics_.load_duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - load_start);
  return update_time;
}

Dumper::Dumper(const Config& initial_config,
               std::unique_ptr<OperationsFactory> rw_factory,
               engine::TaskProcessor& fs_task_processor,
               dynamic_config::Source config_source,
               utils::statistics::Storage& statistics_storage,
               testsuite::DumpControl& dump_control, DumpableEntity& dumpable)
    : impl_(initial_config, std::move(rw_factory), fs_task_processor,
            config_source, statistics_storage, dump_control, dumpable, *this) {}

Dumper::Dumper(const components::ComponentConfig& config,
               const components::ComponentContext& context,
               DumpableEntity& dumpable)
    : Dumper(ParseConfig(config, context), context, dumpable) {}

Dumper::Dumper(const Config& initial_config,
               const components::ComponentContext& context,
               DumpableEntity& dumpable)
    : impl_(initial_config, CreateOperationsFactory(initial_config, context),
            context.GetTaskProcessor(initial_config.fs_task_processor),
            context.FindComponent<components::DynamicConfig>().GetSource(),
            context.FindComponent<components::StatisticsStorage>().GetStorage(),
            context.FindComponent<components::TestsuiteSupport>()
                .GetDumpControl(),
            dumpable, *this) {}

Dumper::~Dumper() = default;

const std::string& Dumper::Name() const { return impl_->Name(); }

std::optional<TimePoint> Dumper::ReadDump() { return impl_->ReadDump(); }

void Dumper::OnUpdateCompleted() { impl_->OnUpdateCompleted(); }

void Dumper::OnUpdateCompleted(TimePoint update_time, UpdateType update_type) {
  impl_->OnUpdateCompleted(update_time, update_type);
}

void Dumper::CancelWriteTaskAndWait() { impl_->CancelWriteTaskAndWait(); }

void Dumper::WriteDumpSyncDebug() { impl_->WriteDumpSyncDebug(); }

void Dumper::ReadDumpDebug() { impl_->ReadDumpDebug(); }

}  // namespace dump

USERVER_NAMESPACE_END
