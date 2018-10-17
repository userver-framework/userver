#pragma once

#include <iosfwd>

namespace storages {
namespace postgres {

/// @brief SQL transaction isolation level
/// @see https://www.postgresql.org/docs/current/static/sql-set-transaction.html
enum class IsolationLevel {
  kReadCommitted,   //!< READ COMMITTED
  kRepeatableRead,  //!< REPEATABLE READ
  kSerializable,    //!< SERIALIZABLE
  kReadUncommitted  //!< READ UNCOMMITTED @warning In Postgres READ UNCOMMITTED
                    //!< is treated as READ COMMITTED
};

std::ostream& operator<<(std::ostream&, IsolationLevel);

/// @brief PostgreSQL transaction options
/// @see https://www.postgresql.org/docs/current/static/sql-set-transaction.html
struct TransactionOptions {
  IsolationLevel isolation_level = IsolationLevel::kReadCommitted;
  bool read_only = false;
  bool deferrable = false;

  constexpr TransactionOptions() = default;
  constexpr explicit TransactionOptions(IsolationLevel lvl)
      : isolation_level{lvl} {}
};

std::ostream& operator<<(std::ostream&, const TransactionOptions&);

}  // namespace postgres
}  // namespace storages
