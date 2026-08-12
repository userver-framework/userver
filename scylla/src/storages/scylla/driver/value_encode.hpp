#pragma once

#include <cstddef>

#include <cassandra.h>

#include <userver/storages/scylla/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {

void BindValue(CassStatement* statement, std::size_t index, const Value& value);

}  // namespace storages::scylla::impl::driver

USERVER_NAMESPACE_END
