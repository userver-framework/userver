#pragma once

#include <exception>

#include <userver/storages/odbc/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

class Connection;

namespace detail {

/// @brief Marks connection broken on unexpected exit (like mysql::impl::BrokenGuard).
class BrokenGuard final {
public:
    explicit BrokenGuard(Connection& connection);
    ~BrokenGuard();

    template <typename Func>
    auto Execute(Func&& func);

private:
    Connection& connection_;
    const int exceptions_on_enter_;
    bool skip_notify_broken_{false};
};

template <typename Func>
auto BrokenGuard::Execute(Func&& func) {
    try {
        return func();
    } catch (const OperationInterrupted&) {
        skip_notify_broken_ = true;
        throw;
    } catch (const StatementError&) {
        /* Query/statement level; keep the connection usable for the pool. */
        skip_notify_broken_ = true;
        throw;
    }
}

}  // namespace detail

}  // namespace storages::odbc

USERVER_NAMESPACE_END
