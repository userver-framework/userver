#pragma once

#include <mariadb/errmsg.h>
#include <mariadb/mysql.h>
#include <mariadb/mysqld_error.h>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

// We use std::size_t interchangeably with unsigned long/unsigned long long
// which libmariadb uses
// TODO : TAXICOMMON-6397, build for 32 bit.
static_assert(sizeof(std::size_t) == sizeof(unsigned long));
static_assert(sizeof(std::size_t) == sizeof(unsigned long long));

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
