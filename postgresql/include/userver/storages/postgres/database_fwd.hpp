#pragma once

/// @file userver/storages/postgres/database_fwd.hpp
/// @brief Forward declarations of Database, DatabasePtr

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

class Database;
using DatabasePtr = std::shared_ptr<Database>;

}  // namespace storages::postgres

USERVER_NAMESPACE_END
