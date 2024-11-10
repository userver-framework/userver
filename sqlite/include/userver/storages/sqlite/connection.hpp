#pragma once

/// @file userver/storages/sqlite/connection.hpp
/// @copybrief @copybrief storages::sqlite::Connection

#include <memory>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/components/component_fwd.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <userver/storages/sqlite/result_set.hpp>
#include <userver/storages/sqlite/query.hpp>
#include <userver/storages/sqlite/transaction.hpp>
#include <userver/storages/sqlite/options.hpp>


USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

namespace settings {
struct SQLiteSettings;
}

class Connection;
using ConnectionPtr = std::shared_ptr<Connection>;

/// @ingroup userver_clients
///
/// @brief Client interface for a SQLite connection.
/// Usually retrieved from components::SQLite
class Connection final {
 public:
  Connection() = default;
  /// @brief Connection constructor
  Connection(const settings::SQLiteSettings& settings,
          const components::ComponentConfig& config);
  /// @brief Connection destructor
  ~Connection();

  template <typename... Args>
  ResultSet Execute(const Query& query [[maybe_unused]], const Args&... args [[maybe_unused]]) {
    return ResultSet{};
  }

  Transaction Begin(std::string name, const TransactionOptions&);
};

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
