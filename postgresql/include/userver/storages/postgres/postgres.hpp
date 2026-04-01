#pragma once

/// @file userver/storages/postgres/postgres.hpp
/// This file is mainly for documentation purposes and inclusion of all headers
/// that are required for working with PostgreSQL userver component.

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/result_set.hpp>
#include <userver/storages/postgres/transaction.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages {
/// @brief Top namespace for uPg driver
///
/// For more information see @ref pg_driver
namespace postgres {
/// @brief uPg input-output.
///
/// Namespace containing classes and functions for defining datatype
/// input-output and specifying mapping between C++ and PostgreSQL types.
namespace io {
/// @brief uPg input-output traits.
///
/// Namespace with metafunctions detecting different type traits needed for
/// PostgreSQL input-output operations
namespace traits {}  // namespace traits
}  // namespace io
}  // namespace postgres
}  // namespace storages

USERVER_NAMESPACE_END
