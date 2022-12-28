#pragma once

#include <mariadb/errmsg.h>
#include <mariadb/mysql.h>
#include <mariadb/mysqld_error.h>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

// I use std::size_t interchangeably with unsigned long which mariadb uses
static_assert(sizeof(std::size_t) == sizeof(unsigned long));

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
