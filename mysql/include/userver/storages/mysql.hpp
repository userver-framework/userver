#pragma once

/// @file userver/storages/mysql.hpp
/// This file is mainly for documentation purposes and inclusion of all headers
/// that are required for working with MySQL µserver component.

#include <userver/cache/base_mysql_cache.hpp>
#include <userver/storages/mysql/cluster.hpp>
#include <userver/storages/mysql/component.hpp>
#include <userver/storages/mysql/cursor_result_set.hpp>

// clang-format off
/// @page mysql_driver MySQL Driver - EXPERIMENTAL
///
/// Disclaimer: current state of the driver is highly experimental, and
/// although APIs are not likely to change drastically, some adjustments may be
/// made in the future.
/// Please also keep in mind that this driver is a community-based effort and is
/// not backed by Yandex expertise nor by Yandex-scale production usage.
///
/// 🐙 **userver** provides access to MySQL databases servers via
/// components::MySQL. The uMySQL driver is asynchronous, and with it one can
/// write queries like this:
/// @snippet storages/tests/unittests/showcase_mysqltest.cpp  uMySQL usage sample - main page
/// No macros, no meta-structs, no boilerplate, just your types used directly. <br>
/// Interested? - read ahead.
///
/// @section legal Legal note
/// Before any further considerations of the uMySQL driver please make yourself
/// aware of the fact that it is designed to work with the <a href = "https://github.com/mariadb-corporation/mariadb-connector-c">
/// mariadb-connector-c</a> library, which is `LGPL2.1` licenced.<br>
/// The driver itself contains no derivative of any portion of the Library, and quoting the
/// <a href="https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html">LGPL2.1 License</a>
/// "Such a work, in isolation, is not a derivative work of the Library, and therefore falls outside the scope of this License.",<br>
/// however linking with `mariadb-connector-c` creates an executable that is a
/// derivative of the `mariadb-connector-c` (because it contains portions of the library),
/// rather than a "work that uses the library".
/// The executable is therefore covered by `LGPL2.1`, and that effectively takes uServers `Apache 2.0` away from the executable,
/// until you comply with `LGPL2.1` linking exception, which is described in section 6 of the `LGPL2.1` license.<br>
/// I am not a lawyer and all these formal nuances are confusing for a mere mortal like me,
/// so i encourage you to consult your own lawyers on the matter.<br>
/// It's also worth mentioning that for the reasons above the uMySQL driver
/// doesn't come with `mariadb-connector-c` linked in out of the box, and it
/// becomes your responsibility to link the resulting executable with the
/// library and comply with its license.
///
/// @section features Features
/// - Connection pooling;
/// - Binary protocol (prepared statements);
/// - Transactions;
/// - Read-only cursors;
/// - Batch Inserts/Upserts (requires MariaDB 10.2.6+);
/// - Variadic template statements parameters passing;
/// - Statement result extraction into C++ types;
/// - Mapping C++ types to native MySQL types;
/// - Userver-specific types: Decimal64, Json;
/// - Nullable for all supported types (via std::optional<T>);
/// - Type safety validation for results extraction, signed vs unsigned
/// validation, nullable vs not-nullable validation;
/// - Seamless integration with userver infrastructure: configuring, logging,
/// metrics etc.;
///
/// @section info More information
/// - For configuring see components::MySQL
/// - For cluster operations see storages::mysql::Cluster
/// - For C++ <-> MySQL mapping see @ref userver_mysql_types
/// - For types extraction of statements results into C++ types see
/// storages::mysql::StatementResultSet
/// - For high-level design and implementation details see @ref userver_mysql_design
// clang-format on

USERVER_NAMESPACE_BEGIN

/// @brief Top namespace for uMySQL driver.
///
/// For more information see @ref mysql_driver
namespace storages::mysql {}

/// @brief Namespace containing helper classes and functions for on-the-flight
/// remapping DbType<->UserType
namespace storages::mysql::convert {}

USERVER_NAMESPACE_END
