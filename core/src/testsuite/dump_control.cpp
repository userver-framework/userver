#include <userver/testsuite/dump_control.hpp>

#include <fmt/format.h>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite {

DumpControl::DumpControl(PeriodicsMode periodics_mode)
    : periodics_mode_(periodics_mode) {}

DumpControl::PeriodicsMode DumpControl::GetPeriodicsMode() const {
  return periodics_mode_;
}

void DumpControl::WriteCacheDumps(
    const std::vector<std::string>& dumper_names) {
  for (const auto& name : dumper_names) {
    FindDumper(name).WriteDumpSyncDebug();
  }
}

void DumpControl::ReadCacheDumps(const std::vector<std::string>& dumper_names) {
  for (const auto& name : dumper_names) {
    FindDumper(name).ReadDumpDebug();
  }
}

void DumpControl::RegisterDumper(dump::Dumper& dumper) {
  auto dumpers = dumpers_.Lock();
  const auto [_, success] = dumpers->try_emplace(dumper.Name(), &dumper);
  UINVARIANT(success,
             fmt::format("Dumper already registered: {}", dumper.Name()));
}

void DumpControl::UnregisterDumper(dump::Dumper& dumper) {
  auto dumpers = dumpers_.Lock();
  const auto removed_count = dumpers->erase(dumper.Name());
  UINVARIANT(removed_count != 0,
             fmt::format("Trying to remove a non-registered dumper: {}",
                         dumper.Name()));
}

dump::Dumper& DumpControl::FindDumper(const std::string& name) const {
  const auto dumpers = dumpers_.Lock();
  const auto iter = dumpers->find(name);
  UINVARIANT(iter != dumpers->end(),
             fmt::format("The requested dumper does not exist: {}", name));
  return *iter->second;
}

DumperRegistrationHolder::DumperRegistrationHolder(DumpControl& control,
                                                   dump::Dumper& dumper)
    : control_(control), dumper_(dumper) {
  control_.RegisterDumper(dumper_);
}

DumperRegistrationHolder::~DumperRegistrationHolder() {
  control_.UnregisterDumper(dumper_);
}

}  // namespace testsuite

USERVER_NAMESPACE_END
