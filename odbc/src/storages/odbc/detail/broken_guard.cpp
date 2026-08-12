#include <storages/odbc/detail/broken_guard.hpp>

#include <storages/odbc/detail/connection.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

BrokenGuard::BrokenGuard(Connection& connection)
    : connection_{connection},
      exceptions_on_enter_{std::uncaught_exceptions()}
{
    if (connection_.IsBroken()) {
        throw ConnectionError("Connection is broken.");
    }
}

BrokenGuard::~BrokenGuard() {
    if (std::uncaught_exceptions() > exceptions_on_enter_) {
        if (!skip_notify_broken_) {
            connection_.NotifyBroken();
        }
    }
}

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
