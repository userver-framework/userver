#pragma once

#include <boost/pfr.hpp>

#include <userver/storages/sqlite/exceptions.hpp>
#include <userver/storages/sqlite/row_types.hpp>
#include <userver/storages/sqlite/sqlite_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

/// @brief Wrapper for executed sqlite3_stmt
class ResultWrapper final {
 public:
  ResultWrapper(StatementBasePtr prepare_statement);
  ~ResultWrapper();

  int RowsAffected() const noexcept;

  int LastInsertRowId() const noexcept;

  bool HasNext() const noexcept;

  bool IsDone() const noexcept;

  void Next() noexcept;

  int ColumnCount() const noexcept;

  template <typename T>
  T FetchNext();

  template <typename RowType>
  RowType FetchNext(RowTag);

  template <typename FieldType>
  FieldType FetchNext(FieldTag);

  template <typename T>
  T ConvertRow();

  template <typename FieldType>
  FieldType GetColumn(int column);

 private:
  std::shared_ptr<StatementBase> prepare_statement_;

  template <typename Tuple, std::size_t... I>
  Tuple ConvertToTupleImpl(std::index_sequence<I...>);

  template <typename Tuple>
  Tuple ConvertToTuple();

  template <typename T>
  T ConvertToAggregate();
};

template <typename T>
T ResultWrapper::FetchNext() {
  return FetchNext<T>(kRowTag);
}

template <typename RowType>
RowType ResultWrapper::FetchNext(RowTag) {
  auto row = ConvertRow<RowType>();
  Next();
  return row;
}

template <typename FieldType>
FieldType ResultWrapper::FetchNext(FieldTag) {
  const int column_count = ColumnCount();
  if (column_count != 1) {
    throw SQLiteException{
        "Result set must have exactly one column for AsVector(FieldTag)"};
  }
  auto column = GetColumn<FieldType>(0);
  Next();
  return column;
}

template <typename T>
T ResultWrapper::ConvertRow() {
  if constexpr (std::is_aggregate_v<T>) {
    return ConvertToAggregate<T>();
  } else {
    return ConvertToTuple<T>();
  }
}

template <typename Tuple, std::size_t... I>
Tuple ResultWrapper::ConvertToTupleImpl(std::index_sequence<I...>) {
  return Tuple{GetColumn<std::tuple_element_t<I, Tuple>>(I)...};
}

template <typename Tuple>
Tuple ResultWrapper::ConvertToTuple() {
  constexpr std::size_t N = std::tuple_size_v<Tuple>;
  return ConvertToTupleImpl<Tuple>(std::make_index_sequence<N>{});
}

template <typename T>
T ResultWrapper::ConvertToAggregate() {
  T instance{};
  int column = 0;
  boost::pfr::for_each_field(instance, [&column, this](auto&& field) {
    using FieldType = std::decay_t<decltype(field)>;
    field = this->GetColumn<FieldType>(column++);
  });
  return instance;
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
