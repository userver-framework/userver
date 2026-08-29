#include <userver/testsuite/dump_control.hpp>

#include <fmt/format.h>

#include <userver/utils/assert.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/resource_scopes.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite {

DumpControl::DumpControl(PeriodicsMode periodics_mode)
    : periodics_mode_(periodics_mode)
{}

Dumper::~Dumper() = default;

DumpControl::PeriodicsMode DumpControl::GetPeriodicsMode() const { return periodics_mode_; }

void DumpControl::WriteCacheDumps(const std::vector<std::string>& dumper_names) {
    for (const auto& name : dumper_names) {
        FindDumper(name).WriteDumpSyncDebug();
    }
}

void DumpControl::ReadCacheDumps(const std::vector<std::string>& dumper_names) {
    for (const auto& name : dumper_names) {
        FindDumper(name).ReadDumpDebug();
    }
}

void DumpControl::RegisterScope(utils::ResourceScopeStorage& scopes, Dumper& dumper) {
    scopes.Register([this, &dumper] {
        RegisterDumper(dumper);
        return utils::FastScopeGuard([this, &dumper]() noexcept { UnregisterDumper(dumper); });
    });
}

void DumpControl::RegisterDumper(Dumper& dumper) {
    auto dumpers = dumpers_.Lock();
    const auto [_, success] = dumpers->try_emplace(dumper.Name(), &dumper);
    UINVARIANT(success, fmt::format("Dumper already registered: {}", dumper.Name()));
}

void DumpControl::UnregisterDumper(Dumper& dumper) noexcept {
    auto dumpers = dumpers_.Lock();
    const auto removed_count = dumpers->erase(dumper.Name());
    UINVARIANT(removed_count != 0, fmt::format("Trying to remove a non-registered dumper: {}", dumper.Name()));
}

Dumper& DumpControl::FindDumper(const std::string& name) const {
    const auto dumpers = dumpers_.Lock();
    const auto iter = dumpers->find(name);
    UINVARIANT(iter != dumpers->end(), fmt::format("The requested dumper does not exist: {}", name));
    return *iter->second;
}

}  // namespace testsuite

USERVER_NAMESPACE_END
