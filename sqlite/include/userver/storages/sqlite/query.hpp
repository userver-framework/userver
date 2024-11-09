#pragma once

/// @file userver/storages/sqlite/query.hpp

#include <optional>
#include <string>

#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

/// @brief Query class, which driver executes.
class Query {
 public:
  /// @brief Strong typedef for query name, one can use named queries to get
  /// better logging experience
  using Name = utils::StrongTypedef<struct NameTag, std::string>;

  /// @brief Query constructor
  Query(const char* statement);

  /// @brief Query constructor
  Query(std::string statement);

  /// @brief Get query statement
  const std::string& GetStatement() const;

 private:
  std::string statement_;
};

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
