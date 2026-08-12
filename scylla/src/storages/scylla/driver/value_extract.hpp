#pragma once

#include <cstddef>
#include <string>

#include <cassandra.h>

#include <userver/storages/scylla/operations.hpp>
#include <userver/storages/scylla/row.hpp>
#include <userver/storages/scylla/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {

Value ExtractValue(const CassValue* cass_val);

Row ExtractRow(const CassResult* result, const CassRow* cass_row);

Rows ExtractAllRows(const CassResult* result);

operations::LwtResult ExtractLwtResult(const CassResult* result);

}  // namespace storages::scylla::impl::driver

USERVER_NAMESPACE_END
