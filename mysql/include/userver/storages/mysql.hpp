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
/// @section features Features
/// - Connection pooling;
/// - Binary protocol (prepared statements);
/// - Transactions;
/// - Read-only cursors;
/// - Batch Inserts/Updates (requires MariaDB 10.2+);
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
/// - For high-level design and implementation details see @ref mysql_driver_design
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
