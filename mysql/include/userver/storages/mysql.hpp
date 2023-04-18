#pragma once

/// @file userver/storages/mysql.hpp
/// This file is mainly for documentation purposes and inclusion of all headers
/// that are required for working with MySQL µserver component.

#include <userver/storages/mysql/cluster.hpp>
#include <userver/storages/mysql/component.hpp>
#include <userver/storages/mysql/cursor_result_set.hpp>
#include <userver/storages/mysql/exceptions.hpp>

// clang-format off
/// @page mysql_driver MySQL Driver - EXPERIMENTAL
///
/// **Disclaimer**: current state of the driver is highly experimental, and
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
/// The uMySQL driver itself contains no derivative of any portion of the underlying
/// <a href="https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html">LGPL2.1 licensed</a>
/// <a href = "https://github.com/mariadb-corporation/mariadb-connector-c">mariadb-connector-c</a> library.
/// However linking with `mariadb-connector-c` may create an executable that is
/// a derivative of the `mariadb-connector-c`, while `LGPL2.1` is incompatible
/// with `Apache 2.0` of userver. Consult your lawyers on the matter. <br>
/// For the reasons above the uMySQL driver doesn't come with
/// `mariadb-connector-c` linked in, and it becomes your responsibility to
/// link with it and comply with the license.
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
/// @section enhancements Planned Enhancements
/// - TLS/SSL - not implemented yet, soon to come;
/// - Automatic primary discovery - not implemented yet,
/// might be implemented soon;
/// - Compression - not implemented yet, may be implemented;
/// - More fine grained configurations - including dynamic configs and exposure
/// of wider spectrum of static settings - likely to be implemented soon
///
/// @section requirements Runtime Requirements
/// Recommended version of libmariadb3 to link against is 3.3.4 or above,
/// because there exists a very nasty bug prior to 3.3.4: <a href="https://jira.mariadb.org/projects/CONC/issues/CONC-622">CONC-622</a>.<br>
/// By default the driver will abort execution if put in situation leading to
/// <a href="https://jira.mariadb.org/projects/CONC/issues/CONC-622">CONC-622</a>,
/// since there's no generally acceptable way to resolve it: <br>
/// either leak memory or straight invoke double free.<br>
/// However, if one is unable to bump libmariadb3 version to 3.3.4 or above and
/// leaking in some hopefully rare cases is acceptable, <br>cmake variable
/// `USERVER_MYSQL_ALLOW_BUGGY_LIBMARIADB` could be set to force the driver
/// to leak memory instead of aborting in such cases.
///
/// @section info More information
/// - For configuring see storages::mysql::Component
/// - For cluster operations see storages::mysql::Cluster
/// - For C++ <-> MySQL mapping see @ref md_en_userver_mysql_supported_types
/// - For typed extraction of statements results into C++ types see
/// storages::mysql::StatementResultSet
/// - For high-level design and implementation details see @ref md_en_userver_mysql_design_and_details
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
