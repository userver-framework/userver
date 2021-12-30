#include <userver/dump/dumper.hpp>

#include <fmt/format.h>
#include <boost/filesystem/operations.hpp>

#include <userver/components/component.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/taxi_config/snapshot.hpp>
#include <userver/taxi_config/source.hpp>
#include <userver/taxi_config/storage/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/tracing/scope_time.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/atomic.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/scope_guard.hpp>
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
  DumpData(std::unique_ptr<OperationsFactory> rw_factory,
           DumpableEntity& dumpable)
      : rw_factory(std::move(rw_factory)), dumpable(dumpable) {
    UASSERT(this->rw_factory);
  }

  const std::unique_ptr<OperationsFactory> rw_factory;
  DumpableEntity& dumpable;
  DumpLocator locator;
  std::optional<UpdateTime> dumped_update_time;
};

struct DumpTaskData {
  engine::TaskWithResult<void> task;
};

struct UpdateData {
  explicit UpdateData(Statistics& statistics)
      : is_current_from_dump(statistics.is_current_from_dump) {}

  std::optional<UpdateTime> update_time;
  std::atomic<bool>& is_current_from_dump;
};

enum class DumpType { kHonorDumpInterval, kForced };

Config ParseConfig(const components::ComponentConfig& config,
                   const components::ComponentContext& context) {
  return Config{
      config.Name(), config[kDump],
      context.FindComponent<components::DumpConfigurator>().GetDumpRoot()};
}

}  // namespace

class Dumper::Impl {
 public:
  Impl(const Config& initial_config,
       std::unique_ptr<OperationsFactory> rw_factory,
       engine::TaskProcessor& fs_task_processor,
       taxi_config::Source config_source,
       utils::statistics::Storage& statistics_storage,
       testsuite::DumpControl& dump_control, DumpableEntity& dumpable,
       Dumper& self);

  ~Impl();

  const std::string& Name() const;

  void WriteDumpAsync();

  std::optional<TimePoint> ReadDump();

  void WriteDumpSyncDebug();

  void ReadDumpDebug();

  void OnUpdateCompleted(TimePoint update_time, UpdateType update_type);

  void CancelWriteTaskAndWait();

 private:
  bool ShouldDump(DumpType type, std::optional<UpdateTime> update_time,
                  DumpTaskData& dump_task_data, const Config& config);

  /// @throws On dump failure
  void DoDump(TimePoint update_time, tracing::ScopeTime& scope,
              DumpData& dump_data, const Config& config);

  enum class DumpOperation { kNewDump, kBumpTime };

  void DumpAsync(DumpOperation operation_type, UpdateTime update_time,
                 DumpTaskData& dump_task_data);

  /// @throws If `type == kForced`, and the Dumper is not ready to write a dump
  void DumpAsyncIfNeeded(DumpType type, DumpTaskData& dump_task_data,
                         const Config& config);

  /// @returns `update_time` of the loaded dump on success, `null` otherwise
  std::optional<TimePoint> LoadFromDump(DumpData& dump_data,
                                        const Config& config);

  void OnConfigUpdate(const taxi_config::Snapshot& config);

  formats::json::Value ExtendStatistics() const;

 private:
  const Config static_config_;
  rcu::Variable<Config> config_;
  engine::TaskProcessor& fs_task_processor_;
  Statistics statistics_;

  concurrent::Variable<DumpData> dump_data_;
  concurrent::Variable<DumpTaskData> dump_task_data_;
  concurrent::Variable<UpdateData> update_data_;

  utils::statistics::Entry statistics_holder_;
  concurrent::AsyncEventSubscriberScope config_subscription_;
  testsuite::DumperRegistrationHolder testsuite_registration_;
};

Dumper::Impl::Impl(const Config& initial_config,
                   std::unique_ptr<OperationsFactory> rw_factory,
                   engine::TaskProcessor& fs_task_processor,
                   taxi_config::Source config_source,
                   utils::statistics::Storage& statistics_storage,
                   testsuite::DumpControl& dump_control,
                   DumpableEntity& dumpable, Dumper& self)
    : static_config_(initial_config),
      config_(static_config_),
      fs_task_processor_(fs_task_processor),
      dump_data_(std::move(rw_factory), dumpable),
      update_data_(statistics_),
      testsuite_registration_(dump_control, self) {
  statistics_holder_ = statistics_storage.RegisterExtender(
      fmt::format("cache.{}.dump", Name()),
      [this](auto&) { return ExtendStatistics(); });
  config_subscription_ = config_source.UpdateAndListen(this, "dump." + Name(),
                                                       &Impl::OnConfigUpdate);
}

Dumper::Impl::~Impl() {
  {
    engine::TaskCancellationBlocker blocker;
    CancelWriteTaskAndWait();
  }
  config_subscription_.Unsubscribe();
  statistics_holder_.Unregister();
}

const std::string& Dumper::Impl::Name() const {
  // It's OK to use `static_config_` here, because `name` is not dynamically
  // configurable.
  return static_config_.name;
}

void Dumper::Impl::WriteDumpAsync() {
  auto dump_task_data = dump_task_data_.Lock();
  const auto config = config_.Read();

  DumpAsyncIfNeeded(DumpType::kHonorDumpInterval, *dump_task_data, *config);
}

std::optional<TimePoint> Dumper::Impl::ReadDump() {
  auto dump_data = dump_data_.Lock();
  const auto config = config_.Read();

  return LoadFromDump(*dump_data, *config);
}

void Dumper::Impl::WriteDumpSyncDebug() {
  auto dump_task_data = dump_task_data_.Lock();
  if (dump_task_data->task.IsValid()) dump_task_data->task.Wait();
  {
    const auto config = config_.Read();
    DumpAsyncIfNeeded(DumpType::kForced, *dump_task_data, *config);
  }
  utils::ScopeGuard clear_dump_task([&] { dump_task_data->task = {}; });
  dump_task_data->task.Get();  // report any exceptions to testsuite
}

void Dumper::Impl::ReadDumpDebug() {
  auto dump_data = dump_data_.Lock();
  const auto config = config_.Read();

  const auto update_time = LoadFromDump(*dump_data, *config);

  if (!update_time) {
    throw Error(fmt::format(
        "{}: failed to read a dump when explicitly requested", Name()));
  }
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
}

void Dumper::Impl::OnConfigUpdate(const taxi_config::Snapshot& config) {
  const auto patch = utils::FindOptional(config[kConfigSet], Name());
  config_.Assign(patch ? static_config_.MergeWith(*patch) : static_config_);
}

formats::json::Value Dumper::Impl::ExtendStatistics() const {
  return formats::json::ValueBuilder{statistics_}.ExtractValue();
}

void Dumper::Impl::CancelWriteTaskAndWait() {
  auto dump_task_data = dump_task_data_.Lock();

  if (dump_task_data->task.IsValid() && !dump_task_data->task.IsFinished()) {
    LOG_WARNING() << Name() << ": stopping a dump task";
    try {
      dump_task_data->task.RequestCancel();
      dump_task_data->task.Get();
    } catch (const std::exception& ex) {
      LOG_ERROR() << Name() << ": exception in dump task. Reason: " << ex;
    }
    dump_task_data->task = {};
  }
}

bool Dumper::Impl::ShouldDump(DumpType type,
                              std::optional<UpdateTime> update_time,
                              DumpTaskData& dump_task_data,
                              const Config& config) {
  if (!config.dumps_enabled) {
    LOG_DEBUG() << Name()
                << ": dump skipped, because dumps are disabled for this dumper";
    return false;
  }

  if (!update_time) {
    LOG_WARNING() << Name()
                  << ": dump skipped, because no successful updates "
                     "have been performed";
    return false;
  }

  if (dump_task_data.task.IsValid() && !dump_task_data.task.IsFinished()) {
    LOG_INFO() << Name()
               << ": dump skipped, because a previous dump "
                  "write is in progress";
    return false;
  }

  // At this point we know that a parallel write task doesn't run (because of
  // the check above) and won't run (because we are holding DumpTaskData).
  const auto dumped_update_time = [&] {
    const auto dump_data = dump_data_.Lock();
    return dump_data->dumped_update_time;
  }();

  if (type == DumpType::kHonorDumpInterval && dumped_update_time &&
      dumped_update_time->last_update >
          update_time->last_update - config.min_dump_interval) {
    LOG_INFO() << Name()
               << ": dump skipped, because dump interval has not passed yet";
    return false;
  }

  return true;
}

void Dumper::Impl::DoDump(TimePoint update_time, tracing::ScopeTime& scope,
                          DumpData& dump_data, const Config& config) {
  const auto dump_start = std::chrono::steady_clock::now();

  std::uint64_t dump_size;
  try {
    auto dump_stats = dump_data.locator.RegisterNewDump(update_time, config);
    const auto& dump_path = dump_stats.full_path;
    auto writer = dump_data.rw_factory->CreateWriter(dump_path, scope);
    dump_data.dumpable.GetAndWrite(*writer);
    writer->Finish();
    dump_size = boost::filesystem::file_size(dump_path);
  } catch (const std::exception& ex) {
    LOG_ERROR() << Name() << ": error while writing a dump. Reason: " << ex;
    throw;
  }

  LOG_INFO() << Name() << ": a new dump has been written";

  statistics_.last_written_size = dump_size;
  statistics_.last_nontrivial_write_duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - dump_start);
  statistics_.last_nontrivial_write_start_time = dump_start;
}

void Dumper::Impl::DumpAsync(DumpOperation operation_type,
                             UpdateTime update_time,
                             DumpTaskData& dump_task_data) {
  if (dump_task_data.task.IsValid()) {
    UASSERT_MSG(dump_task_data.task.IsFinished(),
                "Another dump write task is already running");

    try {
      dump_task_data.task.Get();
    } catch (const std::exception& ex) {
      LOG_ERROR() << Name()
                  << ": error from writing a previous dump. Reason: " << ex;
    }
    dump_task_data.task = {};
  }

  dump_task_data.task = utils::Async(
      fs_task_processor_, "write-dump", [this, operation_type, update_time] {
        try {
          auto dump_data = dump_data_.Lock();

          auto scope_time = tracing::Span::CurrentSpan().CreateScopeTime(
              "write-dump/" + Name());

          const auto config = config_.Read();

          switch (operation_type) {
            case DumpOperation::kNewDump: {
              dump_data->locator.Cleanup(*config);
              DoDump(update_time.last_update, scope_time, *dump_data, *config);
              break;
            }
            case DumpOperation::kBumpTime: {
              UASSERT(dump_data->dumped_update_time);
              const auto dumped_update_time =
                  dump_data->dumped_update_time->last_update;
              if (!dump_data->locator.BumpDumpTime(
                      dumped_update_time, update_time.last_update, *config)) {
                DoDump(update_time.last_update, scope_time, *dump_data,
                       *config);
              }
              break;
            }
          }

          dump_data->dumped_update_time = update_time;
        } catch (const std::exception& ex) {
          LOG_ERROR() << Name() << ": failed to write a dump. Reason: " << ex;
          throw;
        }
      });
}

void Dumper::Impl::DumpAsyncIfNeeded(DumpType type,
                                     DumpTaskData& dump_task_data,
                                     const Config& config) {
  LOG_DEBUG() << Name() << ": requested to write a dump";

  const auto update_time = [&] {
    const auto update_data = update_data_.Lock();
    return update_data->update_time;
  }();

  if (!ShouldDump(type, update_time, dump_task_data, config)) {
    if (type == DumpType::kForced) {
      throw Error(fmt::format(
          "{}: not ready to write a dump, see logs for details", Name()));
    }
    return;
  }

  // At this point we know that a parallel write task doesn't run (because of
  // the check above) and won't run (because we are holding DumpTaskData).
  const auto dumped_update_time = [&] {
    const auto dump_data = dump_data_.Lock();
    return dump_data->dumped_update_time;
  }();

  auto operation = DumpOperation::kNewDump;
  if (dumped_update_time && dumped_update_time->last_modifying_update ==
                                update_time->last_modifying_update) {
    // If nothing has been updated since the last time, skip the serialization
    // and dump processes by just renaming the dump file.
    LOG_DEBUG() << Name() << ": skipped dump, because nothing has been updated";
    operation = DumpOperation::kBumpTime;
  }

  UASSERT_MSG(update_time, "ShouldDump checks that 'update_time != null'");
  DumpAsync(operation, *update_time, dump_task_data);
}

std::optional<TimePoint> Dumper::Impl::LoadFromDump(DumpData& dump_data,
                                                    const Config& config) {
  tracing::Span span("load-from-dump/" + Name());
  const auto load_start = std::chrono::steady_clock::now();

  if (!config.dumps_enabled) {
    LOG_DEBUG() << Name()
                << ": could not load a dump, because dumps are disabled for "
                   "this dumper";
    return {};
  }

  const std::optional<TimePoint> update_time =
      utils::Async(fs_task_processor_, "read-dump", [&] {
        try {
          auto dump_stats = dump_data.locator.GetLatestDump(config);
          if (!dump_stats) return std::optional<TimePoint>{};

          auto reader =
              dump_data.rw_factory->CreateReader(dump_stats->full_path);
          dump_data.dumpable.ReadAndSet(*reader);
          reader->Finish();

          return std::optional{dump_stats->update_time};
        } catch (const std::exception& ex) {
          LOG_ERROR() << Name()
                      << ": error while reading a dump. Reason: " << ex;
          return std::optional<TimePoint>{};
        }
      }).Get();

  if (!update_time) return {};
  const UpdateTime update_times{*update_time, *update_time};

  LOG_INFO() << Name() << ": a dump has been loaded successfully";
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
               taxi_config::Source config_source,
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
            context.FindComponent<components::TaxiConfig>().GetSource(),
            context.FindComponent<components::StatisticsStorage>().GetStorage(),
            context.FindComponent<components::TestsuiteSupport>()
                .GetDumpControl(),
            dumpable, *this) {}

Dumper::~Dumper() = default;

const std::string& Dumper::Name() const { return impl_->Name(); }

void Dumper::WriteDumpAsync() { impl_->WriteDumpAsync(); }

std::optional<TimePoint> Dumper::ReadDump() { return impl_->ReadDump(); }

void Dumper::WriteDumpSyncDebug() { impl_->WriteDumpSyncDebug(); }

void Dumper::ReadDumpDebug() { impl_->ReadDumpDebug(); }

void Dumper::OnUpdateCompleted(TimePoint update_time, UpdateType update_type) {
  impl_->OnUpdateCompleted(update_time, update_type);
}

void Dumper::SetModifiedAndWriteAsync() {
  impl_->OnUpdateCompleted(
      std::chrono::round<TimePoint::duration>(utils::datetime::Now()),
      UpdateType::kModified);
  impl_->WriteDumpAsync();
}

void Dumper::CancelWriteTaskAndWait() { impl_->CancelWriteTaskAndWait(); }

}  // namespace dump

USERVER_NAMESPACE_END
