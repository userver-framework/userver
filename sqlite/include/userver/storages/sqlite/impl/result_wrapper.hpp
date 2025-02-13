#pragma once

#include <memory>

#include <sqlite3.h>
#include <boost/pfr.hpp>

#include <userver/storages/sqlite/row_types.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

class FieldExtractorBase {
 public:
  virtual ~FieldExtractorBase() = default;
  virtual int32_t GetInt32Column(int column) const noexcept = 0;
  virtual uint32_t GetUInt32Column(int column) const noexcept = 0;
  virtual int64_t GetInt64Column(int column) const noexcept = 0;
  virtual double GetDoubleColumn(int column) const noexcept = 0;
  virtual const char* GetCStringColumn(int column) const noexcept = 0;
  virtual std::string GetStringColumn(int column) const noexcept = 0;
  virtual const void* GetBlobColumn(int column) const noexcept = 0;
  virtual std::vector<uint8_t> GetBytesColumn(int column) const noexcept = 0;
};

class FieldExtractor : public FieldExtractorBase {
 public:
  explicit FieldExtractor(std::shared_ptr<sqlite3_stmt> stmt)
      : stmt_(std::move(stmt)) {};
  ~FieldExtractor() override = default;
  int32_t GetInt32Column(int column) const noexcept override;
  uint32_t GetUInt32Column(int column) const noexcept override;
  int64_t GetInt64Column(int column) const noexcept override;
  double GetDoubleColumn(int column) const noexcept override;
  const char* GetCStringColumn(int column) const noexcept override;
  std::string GetStringColumn(int column) const noexcept override;
  const void* GetBlobColumn(int column) const noexcept override;
  std::vector<uint8_t> GetBytesColumn(int column) const noexcept override;

 private:
  std::shared_ptr<sqlite3_stmt> stmt_;
};

class ResultWrapperBase {
 public:
  explicit ResultWrapperBase(std::shared_ptr<FieldExtractorBase> fieldExtractor)
      : fieldExtractor_(fieldExtractor) {}
  virtual ~ResultWrapperBase() = default;
  virtual int RowsAffected() const noexcept = 0;
  virtual int LastInsertRowId() const noexcept = 0;
  virtual bool HasNext() const noexcept = 0;
  virtual bool IsDone() const noexcept = 0;
  virtual void Next() noexcept = 0;
  virtual int ColumnCount() const noexcept = 0;

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

  template <typename FieldType>
  FieldType GetColumn(int column);

  template <typename T>
  T ConvertRow() {
    if constexpr (std::is_aggregate_v<T>) {
      return ConvertToAggregate<T>();
    } else {
      return ConvertToTuple<T>();
    }
  }

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
      field = GetColumn<FieldType>(column++);
    });
    return instance;
  }

 private:
  std::shared_ptr<FieldExtractorBase> fieldExtractor_;
};

template <>
inline int32_t ResultWrapperBase::GetColumn<int32_t>(int column) {
  return fieldExtractor_->GetInt32Column(column);
}

template <>
inline uint32_t ResultWrapperBase::GetColumn<uint32_t>(int column) {
  return fieldExtractor_->GetUInt32Column(column);
}

template <>
inline int64_t ResultWrapperBase::GetColumn<int64_t>(int column) {
  return fieldExtractor_->GetInt64Column(column);
}

template <>
inline double ResultWrapperBase::GetColumn<double>(int column) {
  return fieldExtractor_->GetDoubleColumn(column);
}

template <>
inline const char* ResultWrapperBase::GetColumn<const char*>(int column) {
  return fieldExtractor_->GetCStringColumn(column);
}

template <>
inline const void* ResultWrapperBase::GetColumn<const void*>(int column) {
  return fieldExtractor_->GetBlobColumn(column);
}

template <>
inline std::string ResultWrapperBase::GetColumn<std::string>(int column) {
  return fieldExtractor_->GetStringColumn(column);
}

template <>
inline std::vector<uint8_t> ResultWrapperBase::GetColumn<std::vector<uint8_t>>(
    int column) {
  return fieldExtractor_->GetBytesColumn(column);
}

/// @brief Wrapper for executed sqlite3_stmt
class ResultWrapper : public ResultWrapperBase {
 public:
  ResultWrapper(std::shared_ptr<FieldExtractorBase> fieldExtractor,
                std::shared_ptr<sqlite3_stmt> stmt, int exec_status);
  ~ResultWrapper() override;

  int RowsAffected() const noexcept override;

  int LastInsertRowId() const noexcept override;

  bool HasNext() const noexcept override;

  bool IsDone() const noexcept override;

  void Next() noexcept override;

  int ColumnCount() const noexcept override;

 private:
  std::shared_ptr<sqlite3_stmt> stmt_;
  int exec_status_;  // TODO: We can get it from sqlite3_errcode, but it maybe
                     // unsafe in a multi-thread environment or with invested
                     // queries
};

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
