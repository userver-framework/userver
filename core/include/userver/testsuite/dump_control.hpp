#pragma once

/// @file userver/testsuite/dump_control.hpp
/// @brief @copybrief testsuite::DumpControl

#include <functional>
#include <unordered_map>
#include <vector>

#include <userver/components/component_fwd.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/dump/dumper.hpp>
#include <userver/utils/not_null.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite {

/// @brief Dumper control interface for testsuite
/// @details All methods are coro-safe.
class DumpControl final {
 public:
  enum class PeriodicsMode { kDisabled, kEnabled };

  explicit DumpControl(PeriodicsMode periodics_mode);

  PeriodicsMode GetPeriodicsMode() const;

  void WriteCacheDumps(const std::vector<std::string>& dumper_names);

  void ReadCacheDumps(const std::vector<std::string>& dumper_names);

 private:
  friend class DumperRegistrationHolder;

  void RegisterDumper(dump::Dumper& dumper);

  void UnregisterDumper(dump::Dumper& dumper);

  dump::Dumper& FindDumper(const std::string& name) const;

  PeriodicsMode periodics_mode_;
  concurrent::Variable<
      std::unordered_map<std::string, utils::NotNull<dump::Dumper*>>>
      dumpers_;
};

/// RAII helper for testsuite registration
class DumperRegistrationHolder final {
 public:
  DumperRegistrationHolder(DumpControl&, dump::Dumper&);

  DumperRegistrationHolder(DumperRegistrationHolder&&) = delete;
  DumperRegistrationHolder& operator=(DumperRegistrationHolder&&) = delete;
  ~DumperRegistrationHolder();

 private:
  DumpControl& control_;
  dump::Dumper& dumper_;
};

}  // namespace testsuite

USERVER_NAMESPACE_END
