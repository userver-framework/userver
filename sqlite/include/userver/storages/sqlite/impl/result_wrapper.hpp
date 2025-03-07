#pragma once

#include <memory>

#include <sqlite3.h>
#include <boost/pfr.hpp>

#include <userver/storages/sqlite/impl/statements_base.hpp>
#include <userver/storages/sqlite/row_types.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

/// @brief Wrapper for executed sqlite3_stmt
class ResultWrapper final {
 public:
  ResultWrapper(std::shared_ptr<StatementBase> prepare_statement);
  ~ResultWrapper();

  int RowsAffected() const noexcept;

  int LastInsertRowId() const noexcept;

  bool HasNext() const noexcept;

  bool IsDone() const noexcept;

  void Next() noexcept;

  int ColumnCount() const noexcept;

  template <typename T>
  T FetchNext() {
    return FetchNext<T>(kRowTag);
  }

  template <typename RowType>
  RowType FetchNext(RowTag) {
    auto row = ConvertRow<RowType>();
    Next();
    return row;
  }

  template <typename FieldType>
  FieldType FetchNext(FieldTag) {
    auto column = GetColumn<FieldType>(0);
    Next();
    return column;
  }

  template <typename T>
  T ConvertRow() {
    if constexpr (std::is_aggregate_v<T>) {
      return ConvertToAggregate<T>();
    } else {
      return ConvertToTuple<T>();
    }
  }

  template <typename FieldType>
  FieldType GetColumn(int column);

 private:
  std::shared_ptr<StatementBase> prepare_statement_;

  template <typename Tuple, std::size_t... I>
  Tuple ConvertToTupleImpl(std::index_sequence<I...>) {
    return Tuple{GetColumn<std::tuple_element_t<I, Tuple>>(I)...};
  }

  template <typename Tuple>
  Tuple ConvertToTuple() {
    constexpr std::size_t N = std::tuple_size_v<Tuple>;
    return ConvertToTupleImpl<Tuple>(std::make_index_sequence<N>{});
  }

  template <typename T>
  T ConvertToAggregate() {
    T instance{};
    int column = 0;
    boost::pfr::for_each_field(instance, [&column, this](auto&& field) {
      using FieldType = std::decay_t<decltype(field)>;
      field = this->GetColumn<FieldType>(column++);
    });
    return instance;
  }
};

template <>
inline int32_t ResultWrapper::GetColumn<int32_t>(int column) {
  return prepare_statement_->GetInt32Column(column);
}

template <>
inline uint32_t ResultWrapper::GetColumn<uint32_t>(int column) {
  return prepare_statement_->GetUInt32Column(column);
}

template <>
inline int64_t ResultWrapper::GetColumn<int64_t>(int column) {
  return prepare_statement_->GetInt64Column(column);
}

template <>
inline double ResultWrapper::GetColumn<double>(int column) {
  return prepare_statement_->GetDoubleColumn(column);
}

template <>
inline const char* ResultWrapper::GetColumn<const char*>(int column) {
  return prepare_statement_->GetCStringColumn(column);
}

template <>
inline const void* ResultWrapper::GetColumn<const void*>(int column) {
  return prepare_statement_->GetBlobColumn(column);
}

template <>
inline std::string ResultWrapper::GetColumn<std::string>(int column) {
  return prepare_statement_->GetStringColumn(column);
}

template <>
inline std::vector<uint8_t> ResultWrapper::GetColumn<std::vector<uint8_t>>(
    int column) {
  return prepare_statement_->GetBytesColumn(column);
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
