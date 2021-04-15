#pragma once

/// @file testsuite/dump_control.hpp
/// @brief @copybrief testsuite::DumpControl

#include <functional>
#include <unordered_map>
#include <vector>

#include <components/component_config.hpp>
#include <dump/dumper.hpp>
#include <engine/mutex.hpp>

namespace testsuite {

/// @brief Dumper control interface for testsuite
/// @details All methods are coro-safe.
class DumpControl final {
 public:
  void WriteCacheDumps(const std::vector<std::string>& dumper_names);

  void ReadCacheDumps(const std::vector<std::string>& dumper_names);

 private:
  friend class DumperRegistrationHolder;

  void RegisterDumper(dump::Dumper& dumper);

  void UnregisterDumper(dump::Dumper& dumper);

  dump::Dumper& FindDumper(const std::string& name) const;

  engine::Mutex mutex_;
  std::unordered_map<std::string, std::reference_wrapper<dump::Dumper>>
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
