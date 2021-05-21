#include <dump/dumper.hpp>

#include <boost/filesystem/operations.hpp>

#include <concurrent/variable.hpp>
#include <dump/dump_locator.hpp>
#include <dump/statistics.hpp>
#include <engine/mutex.hpp>
#include <engine/task/cancel.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/task/task_with_result.hpp>
#include <formats/json/value_builder.hpp>
#include <logging/log.hpp>
#include <testsuite/dump_control.hpp>
#include <utils/async.hpp>
#include <utils/atomic.hpp>
#include <utils/prof.hpp>
#include <utils/scope_guard.hpp>

namespace dump {

void ThrowDumpUnimplemented(const std::string& name) {
  YTX_INVARIANT(false, fmt::format("Dumps are unimplemented for {}. "
                                   "See dump::Read, dump::Write",
                                   name));
}

struct Dumper::UpdateTime final {
  TimePoint last_update;
  TimePoint last_modifying_update;
};

struct Dumper::DumpData {
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

struct Dumper::DumpTaskData {
  engine::TaskWithResult<void> task;
};

struct Dumper::UpdateData {
  explicit UpdateData(Statistics& statistics)
      : is_current_from_dump(statistics.is_current_from_dump) {}

  std::optional<UpdateTime> update_time;
  std::atomic<bool>& is_current_from_dump;
};

struct Dumper::Impl {
  Impl(const Config& config, std::unique_ptr<OperationsFactory> rw_factory,
       engine::TaskProcessor& fs_task_processor,
       testsuite::DumpControl& dump_control, DumpableEntity& dumpable,
       Dumper& self)
      : static_config(config),
        config(static_config),
        fs_task_processor(fs_task_processor),
        dump_control(dump_control),
        dump_data(std::move(rw_factory), dumpable),
        update_data(statistics),
        testsuite_registration(dump_control, self) {}

  const Config static_config;
  rcu::Variable<Config> config;
  engine::TaskProcessor& fs_task_processor;
  testsuite::DumpControl& dump_control;
  Statistics statistics;

  concurrent::Variable<DumpData> dump_data;
  concurrent::Variable<DumpTaskData> dump_task_data;
  concurrent::Variable<UpdateData> update_data;

  testsuite::DumperRegistrationHolder testsuite_registration;
};

Dumper::Dumper(const Config& config,
               std::unique_ptr<OperationsFactory> rw_factory,
               engine::TaskProcessor& fs_task_processor,
               testsuite::DumpControl& dump_control, DumpableEntity& dumpable)
    : impl_(config, std::move(rw_factory), fs_task_processor, dump_control,
            dumpable, *this) {}

Dumper::~Dumper() {
  engine::TaskCancellationBlocker blocker;
  CancelWriteTaskAndWait();
}

const std::string& Dumper::Name() const {
  // It's OK to use `static_config_` here, because `name` is not dynamically
  // configurable.
  return impl_->static_config.name;
}

void Dumper::WriteDumpAsync() {
  auto dump_task_data = impl_->dump_task_data.Lock();
  const auto config = impl_->config.Read();

  DumpAsyncIfNeeded(DumpType::kHonorDumpInterval, *dump_task_data, *config);
}

std::optional<TimePoint> Dumper::ReadDump() {
  auto dump_data = impl_->dump_data.Lock();
  const auto config = impl_->config.Read();

  return LoadFromDump(*dump_data, *config);
}

void Dumper::WriteDumpSyncDebug() {
  auto dump_task_data = impl_->dump_task_data.Lock();
  if (dump_task_data->task.IsValid()) dump_task_data->task.Wait();
  {
    const auto config = impl_->config.Read();
    DumpAsyncIfNeeded(DumpType::kForced, *dump_task_data, *config);
  }
  utils::ScopeGuard clear_dump_task([&] { dump_task_data->task = {}; });
  dump_task_data->task.Get();  // report any exceptions to testsuite
}

void Dumper::ReadDumpDebug() {
  auto dump_data = impl_->dump_data.Lock();
  const auto config = impl_->config.Read();

  const auto update_time = LoadFromDump(*dump_data, *config);

  if (!update_time) {
    throw Error(fmt::format(
        "{}: failed to read a dump when explicitly requested", Name()));
  }
}

void Dumper::OnUpdateCompleted(TimePoint update_time,
                               bool has_changes_since_last_update) {
  auto update_data = impl_->update_data.Lock();

  if (has_changes_since_last_update) {
    update_data->update_time = {update_time, update_time};
    update_data->is_current_from_dump = false;
  } else {
    YTX_INVARIANT(update_data->update_time,
                  "The initial update must be marked as a modifying update");
    update_data->update_time->last_update = update_time;
  }
}

void Dumper::SetConfigPatch(const std::optional<ConfigPatch>& patch) {
  impl_->config.Assign(patch ? impl_->static_config.MergeWith(*patch)
                             : impl_->static_config);
}

rcu::ReadablePtr<Config> Dumper::GetConfig() const {
  return impl_->config.Read();
}

formats::json::Value Dumper::ExtendStatistics() const {
  return formats::json::ValueBuilder{impl_->statistics}.ExtractValue();
}

void Dumper::CancelWriteTaskAndWait() {
  auto dump_task_data = impl_->dump_task_data.Lock();

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

bool Dumper::ShouldDump(DumpType type, std::optional<UpdateTime> update_time,
                        DumpTaskData& dump_task_data, const Config& config) {
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
    const auto dump_data = impl_->dump_data.Lock();
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

void Dumper::DoDump(TimePoint update_time, ScopeTime& scope,
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

  impl_->statistics.last_written_size = dump_size;
  impl_->statistics.last_nontrivial_write_duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - dump_start);
  impl_->statistics.last_nontrivial_write_start_time = dump_start;
}

void Dumper::DumpAsync(DumpOperation operation_type, UpdateTime update_time,
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
      impl_->fs_task_processor, "write-dump",
      [this, operation_type, update_time] {
        try {
          auto dump_data = impl_->dump_data.Lock();

          auto scope_time = tracing::Span::CurrentSpan().CreateScopeTime(
              "write-dump/" + Name());

          const auto config = impl_->config.Read();

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

void Dumper::DumpAsyncIfNeeded(DumpType type, DumpTaskData& dump_task_data,
                               const Config& config) {
  LOG_DEBUG() << Name() << ": requested to write a dump";

  const auto update_time = [&] {
    const auto update_data = impl_->update_data.Lock();
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
    const auto dump_data = impl_->dump_data.Lock();
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

std::optional<TimePoint> Dumper::LoadFromDump(DumpData& dump_data,
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
      utils::Async(impl_->fs_task_processor, "read-dump", [&] {
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
    auto update_data = impl_->update_data.Lock();
    update_data->update_time = update_times;
    update_data->is_current_from_dump = true;
  }
  // So that we don't attempt to write the dump we've just read
  dump_data.dumped_update_time = update_times;

  impl_->statistics.is_loaded = true;
  impl_->statistics.load_duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - load_start);
  return update_time;
}

}  // namespace dump
