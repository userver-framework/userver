#pragma once

/// @file userver/ydb/dist_lock/component_base.hpp
/// @brief @copybrief ydb::DistLockComponentBase

#include <optional>
#include <string>

#include <userver/components/component_base.hpp>
#include <userver/ydb/dist_lock/worker.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite {
class TestsuiteTasks;
}  // namespace testsuite

namespace ydb {

/// @ingroup userver_components userver_base_classes
///
/// @brief Base class for YDB-based distlock worker components
///
/// A component that implements a distlock with lock in YDB. Inherit from
/// DistLockComponentBase and implement DoWork(). Lock options are configured
/// in static config.
///
/// The class must be used for infinite loop jobs.
///
/// ## Static configuration example:
///
/// @snippet ydb/functional_tests/basic/static_config.yaml  sample-dist-lock
///
/// ## Static options of ydb::DistLockComponentBase :
/// @include{doc} scripts/docs/en/components_schema/ydb/src/ydb/dist_lock/component_base.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
///
/// @see @ref scripts/docs/en/userver/periodics.md
class DistLockComponentBase : public components::ComponentBase {
public:
    DistLockComponentBase(const components::ComponentConfig&, const components::ComponentContext&);

    /// @brief Checks if the the current service instance owns the lock.
    ///
    /// Useful for:
    ///
    /// 1. Writing metrics only on the primary (for the given distlock) host.
    ///
    /// 2. Running on-demand work, e.g. in handlers, only on the primary host.
    ///    The work must be short-lived (single requests with small timeout),
    ///    otherwise brain split
    ///    (where multiple hosts consider themselves primary) is possible.
    bool OwnsLock() const noexcept;

    static yaml_config::Schema GetStaticConfigSchema();

protected:
    /// Override this function with anything that must be done under the lock.
    ///
    /// @note `DoWork` must honour task cancellation and stop ASAP when
    /// it is cancelled, otherwise brain split is possible (IOW, two different
    /// users do work assuming both of them hold the lock, which is not true).
    ///
    /// @note When DoWork exits for any reason, the lock is dropped, then after
    /// `restart-delay` the lock is attempted to be re-acquired (but by that time
    /// another host likely acquires the lock).
    ///
    /// ## Example implementation
    ///
    /// @snippet ydb/functional_tests/basic/ydb_service.cpp  DoWork
    virtual void DoWork() = 0;

    /// Override this function to provide a custom testsuite handler, e.g. one
    /// that does not contain a "while not cancelled" loop.
    /// Calls `DoWork` by default.
    ///
    /// In testsuite runs, the normal DoWork execution disabled by default.
    /// There is an API to call DoWorkTestsuite from testsuite, imitating waiting
    /// until DoWork runs:
    ///
    /// @snippet ydb/functional_tests/basic/tests/test_distlock.py  run
    virtual void DoWorkTestsuite() { DoWork(); }

    /// Must be called at the end of the constructor of the derived component.
    void Start();

    /// Must be called in the destructor of the derived component.
    void Stop() noexcept;

private:
    testsuite::TestsuiteTasks& testsuite_tasks_;
    const std::string testsuite_task_name_;

    // Worker contains a task that may refer to other fields, so it must be right
    // before subscriptions. Add new fields above this comment.
    std::optional<DistLockedWorker> worker_;
};

}  // namespace ydb

USERVER_NAMESPACE_END
