#pragma once

/// @file userver/dump/dumper.hpp
/// @brief @copybrief dump::Dumper

#include <chrono>
#include <memory>
#include <optional>
#include <string>

#include <userver/components/component_fwd.hpp>
#include <userver/dump/operations.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/taxi_config/config_fwd.hpp>
#include <userver/utils/fast_pimpl.hpp>

namespace utils::statistics {
class Storage;
}  // namespace utils::statistics

namespace testsuite {
class DumpControl;
}  // namespace testsuite

/// Dumping of cache-like components
namespace dump {

struct Config;
class OperationsFactory;
extern const std::string_view kDump;

using TimePoint = std::chrono::time_point<std::chrono::system_clock,
                                          std::chrono::microseconds>;

[[noreturn]] void ThrowDumpUnimplemented(const std::string& name);

/// A dynamically dispatched equivalent of `kDumpable` "concept". Unlike
/// with ADL-found `Write`/`Read`, the methods are guaranteed not to be called
/// in parallel.
class DumpableEntity {
 public:
  virtual ~DumpableEntity();

  virtual void GetAndWrite(dump::Writer& writer) const = 0;

  virtual void ReadAndSet(dump::Reader& reader) = 0;
};

enum class UpdateType {
  /// Some new data has appeared since the last update. `Dumper` will write it
  /// on the next `WriteDumpAsync` call, or as specified by the config.
  kModified,

  /// There is no new data, but we have verified that the old data is
  /// up-to-date. `Dumper` will bump the dump modification time to `now`.
  kAlreadyUpToDate,
};

// clang-format off
/// @brief Manages dumps of a cache-like component
///
/// The class is thread-safe.
///
/// Used in `components::CachingComponentBase`.
///
/// Automatically subscribes to:
/// - dynamic config updates from `USERVER_DUMPS` under `dumper_name`
/// - statistics under `cache.{dumper_name}.dump`
///
/// Dumps will be stored in `{dump-root}/{dumper_name}`, where `dump-root` is
/// taken from `components::DumpConfigurator`.
///
/// Here, `dumper_name` is the name of the parent component.
///
/// ## Dynamic config
/// * @ref USERVER_DUMPS
///
/// ## Static config
/// Name | Type | Description | Default value
/// ---- | ---- | ----------- | -------------
/// `enable` | `boolean` | Whether this `Dumper` should actually read and write dumps | (required)
/// `world-readable` | `boolean` | If `true`, dumps are created with access `0444`, otherwise with access `0400` | (required)
/// `format-version` | `integer` | Allows to ignore dumps written with an obsolete `format-version` | (required)
/// `max-age` | optional `string` (duration) | Overdue dumps are ignored | null
/// `max-count` | optional `integer` | Old dumps over the limit are removed from disk | `1`
/// `min-interval` | `string` (duration) | `WriteDumpAsync` calls performed in a fast succession are ignored | `0s`
/// `fs-task-processor` | `string` | `TaskProcessor` for blocking disk IO | `fs-task-processor`
/// `encrypted` | `boolean` | Whether to encrypt the dump | `false`
///
/// ## Sample usage
/// @sample core/src/dump/dumper_test.cpp  Sample Dumper usage
///
/// @see components::DumpConfigurator
// clang-format on
class Dumper final {
 public:
  /// @brief The primary constructor for when `Dumper` is stored in a component
  /// @note `dumpable` must outlive this `Dumper`
  Dumper(const components::ComponentConfig& config,
         const components::ComponentContext& context, DumpableEntity& dumpable);

  /// For internal use only
  Dumper(const Config& initial_config,
         std::unique_ptr<OperationsFactory> rw_factory,
         engine::TaskProcessor& fs_task_processor,
         taxi_config::Source config_source,
         utils::statistics::Storage& statistics_storage,
         testsuite::DumpControl& dump_control, DumpableEntity& dumpable);

  Dumper(Dumper&&) = delete;
  Dumper& operator=(Dumper&&) = delete;
  ~Dumper();

  const std::string& Name() const;

  /// @brief Write data to a dump if data has been modified
  ///
  /// `Dumper` has NO `PeriodicTask`s running in the background. All writes must
  /// be performed explicitly by the user.
  ///
  /// The dump is only written if:
  /// 1. data update has been reported via `OnUpdateCompleted` since the last
  ///    written dump
  /// 2. dumps are currently enabled
  /// 3. `min-interval` time has passed
  ///
  /// It is a good idea to call `WriteDumpAsync` after each update.
  ///
  /// @note Catches and logs any exceptions related to write operation failure
  /// @see OnUpdateCompleted
  void WriteDumpAsync();

  /// @brief Read data from a dump, if any
  /// @note Catches and logs any exceptions related to read operation failure
  /// @returns `update_time` of the loaded dump on success, `null` otherwise
  std::optional<TimePoint> ReadDump();

  /// @brief Forces the `Dumper` to write a dump synchronously
  /// @throws std::exception if the `Dumper` failed to write a dump
  void WriteDumpSyncDebug();

  /// @brief Forces the `Dumper` to read from a dump synchronously
  /// @throws std::exception if the `Dumper` failed to read a dump
  void ReadDumpDebug();

  /// @brief Notifies the `Dumper` of an update in the `DumpableEntity`
  ///
  /// Must be called at some point before a `WriteDumpAsync` call,
  /// otherwise no dump will be written.
  void OnUpdateCompleted(TimePoint update_time, UpdateType update_type);

  /// @brief Equivalent to `OnUpdateCompleted(now, true) + `WriteDumpAsync()`
  void SetModifiedAndWriteAsync();

  /// @brief Cancel and wait for the task launched by `WriteDumpAsync`, if any
  ///
  /// The task is automatically cancelled and waited for in the destructor. This
  /// method must be called if the `DumpableEntity` may start its destruction
  /// before the `Dumper` is destroyed.
  void CancelWriteTaskAndWait();

 private:
  Dumper(const Config& initial_config,
         const components::ComponentContext& context, DumpableEntity& dumpable);

  class Impl;
  utils::FastPimpl<Impl, 896, 8> impl_;
};

}  // namespace dump
