#pragma once

/// @file userver/testsuite/dump_control.hpp
/// @brief @copybrief testsuite::DumpControl

#include <string>
#include <unordered_map>
#include <vector>

#include <userver/components/component_fwd.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/utils/not_null.hpp>
#include <userver/utils/resource_scopes_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite {

/// @brief Testsuite-facing API of a dump manager.
class Dumper {
public:
    virtual ~Dumper();

    virtual const std::string& Name() const = 0;

    virtual void WriteDumpSyncDebug() = 0;

    virtual void ReadDumpDebug() = 0;
};

/// @brief Dumper control interface for testsuite
/// @details All methods are coro-safe.
class DumpControl final {
public:
    enum class PeriodicsMode { kDisabled, kEnabled };

    explicit DumpControl(PeriodicsMode periodics_mode);

    PeriodicsMode GetPeriodicsMode() const;

    void WriteCacheDumps(const std::vector<std::string>& dumper_names);

    void ReadCacheDumps(const std::vector<std::string>& dumper_names);

    /// @brief Registers a dumper bound to @a scopes.
    ///
    /// The dumper is registered after construction of the object that owns
    /// @a scopes and is unregistered just before that object is destroyed.
    void RegisterScope(utils::ResourceScopeStorage& scopes, Dumper& dumper);

private:
    void RegisterDumper(Dumper& dumper);

    void UnregisterDumper(Dumper& dumper) noexcept;

    Dumper& FindDumper(const std::string& name) const;

    PeriodicsMode periodics_mode_;
    concurrent::Variable<std::unordered_map<std::string, utils::NotNull<Dumper*>>> dumpers_;
};

}  // namespace testsuite

USERVER_NAMESPACE_END
