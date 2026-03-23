#pragma once

/// @file userver/storages/scylla.hpp
/// @brief Include-all header for ScyllaDB client

#include <userver/storages/scylla/component.hpp>
#include <userver/storages/scylla/exception.hpp>
#include <userver/storages/scylla/session.hpp>
#include <userver/storages/scylla/session_config.hpp>
#include <userver/storages/scylla/table.hpp>
#include <userver/storages/scylla/table_impl.hpp>
#include <userver/storages/scylla/operations.hpp>

USERVER_NAMESPACE_BEGIN

// ScyllaDB client
namespace storages::scylla {}

USERVER_NAMESPACE_END