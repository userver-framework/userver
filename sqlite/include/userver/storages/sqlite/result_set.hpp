#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include <userver/storages/sqlite/exceptions.hpp>
#include <userver/storages/sqlite/execution_result.hpp>
#include <userver/storages/sqlite/impl/result_wrapper.hpp>
#include <userver/storages/sqlite/row_types.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

class ResultSet {
 public:
  using size_type = std::size_t;

  explicit ResultSet(std::shared_ptr<impl::ResultWrapper> pimpl);

  ResultSet(const ResultSet& other) = delete;
  ResultSet(ResultSet&& other) noexcept;
  ResultSet& operator=(ResultSet&&) noexcept;

  ~ResultSet();

  // clang-format off
  /// @brief Parse statement result set as std::vector<T>.
  /// T is expected to be an aggregate of supported types.
  ///
  /// UINVARIANTs on columns count mismatch or types mismatch.
  ///
  // clang-format on
  template <typename T>
  std::vector<T> AsVector() &&;

  // clang-format off
  /// @brief Parse statement result set as std::vector<T>.
  /// Result set is expected to have a single column, `T` is expected to be one
  /// of supported types.
  ///
  /// UINVARIANTs on columns count not being equal to 1 or type mismatch.
  ///
  // clang-format on
  template <typename T>
  std::vector<T> AsVector(FieldTag) &&;

  // clang-format off
  /// @brief Parse statement result as T.
  /// Result set is expected to have a single row, `T` is expected to be an
  /// aggregate of supported types.
  ///
  /// UINVARIANTs on columns count mismatch or types mismatch.
  /// throws if result set is empty or contains more than one row.
  ///
  // clang-format on
  template <typename T>
  T AsSingleRow() &&;

  // clang-format off
  /// @brief Parse statement result as T.
  /// Result set is expected to have a single row and a single column,
  /// `T` is expected to be one of supported types.
  ///
  /// UINVARIANTs on columns count not being equal to 1 or type mismatch.
  /// throws if result set is empty of contains more than one row.
  ///
  // clang-format on
  template <typename T>
  T AsSingleField() &&;

  // clang-format off
  /// @brief Parse statement result as std::optional<T>.
  /// Result set is expected to have not more than one row,
  /// `T` is expected to be an aggregate of supported types.
  ///
  /// UINVARIANTs on columns count mismatch or types mismatch.
  /// throws if result set contains more than one row.
  ///
  // clang-format on
  template <typename T>
  std::optional<T> AsOptionalSingleRow() &&;

  // clang-format off
  /// @brief Parse statement result as T.
  /// Result set is expected to have not more than one row,
  /// `T` is expected to be one of supported types.
  ///
  /// UINVARIANTs on columns count not being equal to 1 or type mismatch.
  /// throws if result set contains more than one row.
  ///
  // clang-format on
  template <typename T>
  std::optional<T> AsOptionalSingleField() &&;

  /// @brief Get statement execution metadata.
  ExecutionResult AsExecutionResult() &&;

 private:
  std::shared_ptr<impl::ResultWrapper> pimpl_;
};

template <typename T>
std::vector<T> ResultSet::AsVector() && {
  // TODO: Add more detailed verification and error description
  // static_assert(is_aggregate_or_tuple_v<T>,
  //               "T must be an aggregate type or tuple-like type");
  std::vector<T> result;
  while (pimpl_->HasNext()) {
    result.emplace_back(pimpl_->FetchNext<T>());
  }
  return result;
}

template <typename T>
std::vector<T> ResultSet::AsVector(FieldTag) && {
  // TODO: Add more detailed verification and error description
  static_assert(std::is_same_v<T, int64_t> || std::is_same_v<T, double> ||
                    std::is_same_v<T, std::string> ||
                    std::is_same_v<T, std::vector<uint8_t>>,
                "Unsupported type for AsVector(FieldTag)");
  const int column_count = pimpl_->ColumnCount();
  if (column_count != 1) {
    throw SQLiteException(
        "Result set must have exactly one column for AsVector(FieldTag)");
  }
  std::vector<T> result;
  while (pimpl_->HasNext()) {
    result.emplace_back(pimpl_->FetchNext<T>(kFieldTag));
  }
  return result;
}

template <typename T>
T ResultSet::AsSingleRow() && {
  // TODO: Add more detailed verification and error description
  // static_assert(std::is_aggregate_v<T> || boost::pfr::tuple_size_v<T> > 0,
  //               "T must be an aggregate type or tuple-like type");

  if (pimpl_->IsDone()) {
    throw SQLiteException("Result set is empty");
  }
  auto result = pimpl_->FetchNext<T>();
  if (pimpl_->HasNext()) {
    throw SQLiteException("Result set contains more than one row");
  }
  return result;
}

template <typename T>
T ResultSet::AsSingleField() && {
  // TODO: Add more detailed verification and error description
  static_assert(std::is_same_v<T, int64_t> || std::is_same_v<T, double> ||
                    std::is_same_v<T, std::string> ||
                    std::is_same_v<T, std::vector<uint8_t>>,
                "T must be one of the supported types: int64_t, double, "
                "std::string, std::vector<uint8_t>");

  if (pimpl_->IsDone()) {
    throw SQLiteException("Result set is empty");
  }
  int column_count = pimpl_->ColumnCount();
  if (column_count != 1) {
    throw SQLiteException("Result set must contain exactly one column");
  }
  auto result = pimpl_->FetchNext<T>(kFieldTag);
  if (pimpl_->HasNext()) {
    throw SQLiteException("Result set contains more than one row");
  }
  return result;
}

template <typename T>
std::optional<T> ResultSet::AsOptionalSingleRow() && {
  // TODO: Add more detailed verification and error description
  // static_assert(std::is_aggregate_v<T> || boost::pfr::tuple_size_v<T> > 0,
  //               "T must be an aggregate type or tuple-like type");

  if (pimpl_->IsDone()) {
    return std::nullopt;
  }
  auto result = pimpl_->FetchNext<T>();
  if (pimpl_->HasNext()) {
    throw SQLiteException("Result set contains more than one row");
  }
  return result;
}

template <typename T>
std::optional<T> ResultSet::AsOptionalSingleField() && {
  // TODO: Add more detailed verification and error description
  static_assert(std::is_same_v<T, int64_t> || std::is_same_v<T, double> ||
                    std::is_same_v<T, std::string> ||
                    std::is_same_v<T, std::vector<uint8_t>>,
                "T must be one of the supported types: int64_t, double, "
                "std::string, std::vector<uint8_t>");

  if (pimpl_->IsDone()) {
    return std::nullopt;
  }
  int column_count = pimpl_->ColumnCount();
  if (column_count != 1) {
    throw SQLiteException("Result set must contain exactly one column");
  }
  auto result = pimpl_->FetchNext<T>(kFieldTag);
  if (pimpl_->HasNext()) {
    throw SQLiteException("Result set contains more than one row");
  }
  return result;
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
