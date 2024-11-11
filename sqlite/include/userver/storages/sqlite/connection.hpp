#pragma once

/// @file userver/storages/sqlite/connection.hpp
/// @copybrief @copybrief storages::sqlite::Connection

#include <memory>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/components/component_fwd.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/query.hpp>
#include <userver/storages/sqlite/result_set.hpp>
#include <userver/storages/sqlite/transaction.hpp>

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
  ResultSet Execute(const Query& query, const Args&... args) const;

  template <typename... Args>
  ResultSet Execute(OptionalCommandControl optional_cc, const Query& query,
                    const Args&... args) const;

  template <typename T>
  ResultSet ExecuteDecompose(const Query& query,
                             const T& row [[maybe_unused]]) const;

  template <typename T>
  ResultSet ExecuteDecompose(OptionalCommandControl optional_cc,
                             const Query& query, const T& row) const;

  template <typename Container>
  ResultSet ExecuteBulk(const Query& query, const Container& params) const;

  template <typename Container>
  ResultSet ExecuteBulk(OptionalCommandControl optional_cc, const Query& query,
                        const Container& params) const;

  Transaction Begin(std::string name, const TransactionOptions&) const;

  Transaction Begin(OptionalCommandControl optional_cc, std::string name,
                    const TransactionOptions&) const;

 private:
  ResultSet DoExecute(OptionalCommandControl optional_cc, const Query& query,
                      std::optional<std::size_t> batch_size) const;
};

template <typename... Args>
ResultSet Connection::Execute(const Query& query, const Args&... args) const {
  return Execute(std::nullopt, query, args...);
}

template <typename... Args>
ResultSet Connection::Execute(OptionalCommandControl optional_cc
                              [[maybe_unused]],
                              const Query& query,
                              const Args&... args [[maybe_unused]]) const {
  return DoExecute(optional_cc, query.GetStatement(), std::nullopt);
}

template <typename T>
ResultSet Connection::ExecuteDecompose(const Query& query,
                                       const T& row [[maybe_unused]]) const {
  return DoExecute(std::nullopt, query.GetStatement(), std::nullopt);
}

template <typename T>
ResultSet Connection::ExecuteDecompose(OptionalCommandControl optional_cc
                                       [[maybe_unused]],
                                       const Query& query,
                                       const T& row [[maybe_unused]]) const {
  return DoExecute(optional_cc, query.GetStatement(), std::nullopt);
}

template <typename Container>
ResultSet Connection::ExecuteBulk(const Query& query,
                                  const Container& params) const {
  return ExecuteBulk(std::nullopt, query, params);
}

template <typename Container>
ResultSet Connection::ExecuteBulk(OptionalCommandControl optional_cc,
                                  const Query& query,
                                  const Container& params
                                  [[maybe_unused]]) const {
  return DoExecute(optional_cc, query.GetStatement(), std::nullopt);
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
