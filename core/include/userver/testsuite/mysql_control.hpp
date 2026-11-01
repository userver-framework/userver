#pragma once

/// @file userver/testsuite/mysql_control.hpp
/// @brief @copybrief testsuite::MysqlControl

USERVER_NAMESPACE_BEGIN

namespace testsuite {

/// @brief Testsuite overrides for MySQL client
struct MysqlControl {
    /// @brief Whether TLS may be required for MySQL connections; if `false`,
    /// `use_secure_connection` of MySQL components is ignored and connections
    /// are not encrypted
    bool is_tls_enabled{true};
};

}  // namespace testsuite

USERVER_NAMESPACE_END
