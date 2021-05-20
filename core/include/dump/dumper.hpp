#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>

#include <dump/config.hpp>
#include <dump/factory.hpp>
#include <engine/task/task_processor_fwd.hpp>
#include <formats/json/value.hpp>
#include <rcu/rcu.hpp>
#include <utils/fast_pimpl.hpp>
#include <utils/prof.hpp>

namespace testsuite {
class DumpControl;
}  // namespace testsuite

namespace dump {

using TimePoint = std::chrono::time_point<std::chrono::system_clock,
                                          std::chrono::microseconds>;

[[noreturn]] void ThrowDumpUnimplemented(const std::string& name);

/// A dynamically dispatched equivalent of `kDumpable` "concept". Unlike
/// with ADL-found `Write`/`Read`, the methods are guaranteed not to be called
/// in parallel.
class DumpableEntity {
 public:
  virtual ~DumpableEntity() = default;

  virtual void GetAndWrite(dump::Writer& writer) const = 0;

  virtual void ReadAndSet(dump::Reader& reader) = 0;
};

/// @brief Manages dumps of a cache-like component
/// @note The class is thread-safe
class Dumper final {
 public:
  /// @note `dumpable` must outlive this `Dumper`
  Dumper(const Config& config, std::unique_ptr<OperationsFactory> rw_factory,
         engine::TaskProcessor& fs_task_processor,
         testsuite::DumpControl& dump_control, DumpableEntity& dumpable);

  Dumper(Dumper&&) = delete;
  Dumper& operator=(Dumper&&) = delete;
  ~Dumper();

  const std::string& Name() const;

  /// @brief Write data to a dump
  /// @note Catches and logs any exceptions related to write operation failure
  /// @see OnUpdateCompleted
  void WriteDumpAsync();

  /// @brief Read data from a dump, if any
  /// @returns `update_time` of the loaded dump on success, `nullopt` otherwise
  std::optional<TimePoint> ReadDump();

  /// @brief Forces the `Dumper` to write a dump synchronously
  /// @throws std::exception if the `Dumper` failed to write a dump
  void WriteDumpSyncDebug();

  /// @brief Forces the `Dumper` to read from a dump synchronously
  /// @throws std::exception if the `Dumper` failed to read a dump
  void ReadDumpDebug();

  /// @brief Must be called at some point before a `WriteDumpAsync` call,
  /// otherwise no dump will be written
  void OnUpdateCompleted(TimePoint update_time,
                         bool has_changes_since_last_dump);

  /// @brief Updates dump config
  /// @note If no config is set, uses static default (from config.yaml)
  void SetConfigPatch(const std::optional<ConfigPatch>& patch);

  /// @brief Get a snapshot of the current dump config
  rcu::ReadablePtr<Config> GetConfig() const;

  /// @returns Dump metrics object, which is usually placed inside `dump` node
  /// of a component's metrics
  formats::json::Value ExtendStatistics() const;

  /// @brief Cancel and wait for the task launched by `WriteDumpAsync`, if any
  /// @details The task is automatically cancelled and waited for
  /// in the destructor. This method must be called if the `DumpableEntity` may
  /// start its destruction before the `Dumper` is destroyed.
  void CancelWriteTaskAndWait();

 private:
  enum class DumpType { kHonorDumpInterval, kForced };

  bool ShouldDump(DumpType type, const Config& config);

  /// @throws On dump failure
  void DoDump(TimePoint update_time, ScopeTime& scope, const Config& config);

  enum class DumpOperation { kNewDump, kBumpTime };

  void DumpAsync(DumpOperation operation_type);

  /// @throws If `type == kForced`, and the Dumper is not ready to write a dump
  void DumpAsyncIfNeeded(DumpType type, const Config& config);

  /// @returns `update_time` of the loaded dump on success, `nullopt` otherwise
  std::optional<TimePoint> LoadFromDump(const Config& config);

 private:
  struct Impl;
  utils::FastPimpl<Impl, 776, 8> impl_;
};

}  // namespace dump
