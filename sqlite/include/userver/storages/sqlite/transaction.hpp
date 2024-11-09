#pragma once

/// @file userver/storages/sqlite/transaction.hpp

#include <userver/storages/sqlite/query.hpp>
#include <userver/storages/sqlite/result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

namespace infra {
class ConnectionPtr;
}

class Transaction final {
 public:
  explicit Transaction() = default;
  ~Transaction();
  Transaction(const Transaction& other) = delete;
  Transaction(Transaction&& other) noexcept;

  template <typename... Args>
  ResultSet Execute(const std::string& query, const Args&... args [[maybe_unused]]) const {
    return DoExecute(query);
  }

  void Commit();

  void Rollback();

 private:
  ResultSet DoExecute(const std::string& query) const;
};

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
