#pragma once

#include <iosfwd>
#include <string>

namespace storages {
namespace postgres {

/*! [Isolation levels] */
/// @brief SQL transaction isolation level
/// @see https://www.postgresql.org/docs/current/static/sql-set-transaction.html
enum class IsolationLevel {
  kReadCommitted,   //!< READ COMMITTED
  kRepeatableRead,  //!< REPEATABLE READ
  kSerializable,    //!< SERIALIZABLE
  kReadUncommitted  //!< READ UNCOMMITTED @warning In Postgres READ UNCOMMITTED
                    //!< is treated as READ COMMITTED
};
/*! [Isolation levels] */

std::ostream& operator<<(std::ostream&, IsolationLevel);

/// @brief PostgreSQL transaction options
///
/// A transaction can be started using all isolation levels and modes
/// supported by PostgreSQL server as specified in it's documentation.
///
/// Default isolation level is READ COMMITTED, default mode is READ WRITE.
/// @code
/// // Read-write read committed transaction.
/// TransactionOptions opts;
/// @endcode
///
/// Transaction class provides constants Transaction::RW, Transaction::RO and
/// Transaction::Deferrable for convenience.
///
/// Other variants can be created with TransactionOptions constructors
/// that are constexpr.
///
/// @see https://www.postgresql.org/docs/current/static/sql-set-transaction.html
struct TransactionOptions {
  /*! [Transaction modes] */
  enum Mode {
    kReadWrite = 0,
    kReadOnly = 1,
    kDeferrable = 3  //!< Deferrable transaction is read only
  };
  /*! [Transaction modes] */
  IsolationLevel isolation_level = IsolationLevel::kReadCommitted;
  Mode mode = kReadWrite;

  constexpr TransactionOptions() = default;
  constexpr explicit TransactionOptions(IsolationLevel lvl)
      : isolation_level{lvl} {}
  constexpr TransactionOptions(IsolationLevel lvl, Mode m)
      : isolation_level{lvl}, mode{m} {}
  constexpr explicit TransactionOptions(Mode m) : mode{m} {}

  bool IsReadOnly() const { return mode & kReadOnly; }

  /// The deferrable property has effect only if the transaction is also
  /// serializable and read only
  static constexpr TransactionOptions Deferrable() {
    return {IsolationLevel::kSerializable, kDeferrable};
  }
};

constexpr inline bool operator==(const TransactionOptions& lhs,
                                 const TransactionOptions& rhs) {
  return lhs.isolation_level == rhs.isolation_level && lhs.mode == rhs.mode;
}
const std::string& BeginStatement(const TransactionOptions&);

}  // namespace postgres
}  // namespace storages
