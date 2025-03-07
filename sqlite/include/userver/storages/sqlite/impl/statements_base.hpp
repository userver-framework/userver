#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <boost/pfr/core.hpp>
#include <boost/pfr/tuple_size.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

class StatementBase {
 public:
  virtual ~StatementBase() = default;

  // Prepare and bind methods
  template <typename... Args>
  void UpdateParamsBindings(const Args&... args);
  template <typename T>
  void UpdateRowAsParamsBindings(const T& row);
  virtual void Bind(const int index, const int32_t value) = 0;
  virtual void Bind(const int index, const int64_t value) = 0;
  virtual void Bind(const int index, const uint32_t value) = 0;
  virtual void Bind(const int index, const uint64_t value) = 0;
  virtual void Bind(const int index, const double value) = 0;
  virtual void Bind(const int index, const std::string& value) = 0;
  virtual void Bind(const int index, const std::string_view value) = 0;
  virtual void Bind(const int index, const char* value, const int size) = 0;
  virtual void Bind(const int index) = 0;

  // Execution methods
  virtual int RowsAffected() const noexcept = 0;
  virtual int LastInsertRowId() const noexcept = 0;
  virtual bool HasNext() const noexcept = 0;
  virtual bool IsDone() const noexcept = 0;
  virtual void Next() noexcept = 0;
  virtual int ColumnCount() const noexcept = 0;

  // Extract result methods
  virtual int32_t GetInt32Column(int column) const noexcept = 0;
  virtual uint32_t GetUInt32Column(int column) const noexcept = 0;
  virtual int64_t GetInt64Column(int column) const noexcept = 0;
  virtual double GetDoubleColumn(int column) const noexcept = 0;
  virtual const char* GetCStringColumn(int column) const noexcept = 0;
  virtual std::string GetStringColumn(int column) const noexcept = 0;
  virtual const void* GetBlobColumn(int column) const noexcept = 0;
  virtual std::vector<uint8_t> GetBytesColumn(int column) const noexcept = 0;
};

template <typename... Args>
void StatementBase::UpdateParamsBindings(const Args&... args) {
  int index = 1;
  (Bind(index++, args), ...);
}

template <typename T>
void StatementBase::UpdateRowAsParamsBindings(const T& row) {
  // TODO: Add more detailed verification and error description
  static_assert(std::is_aggregate_v<T> || boost::pfr::tuple_size_v<T> > 0,
                "T must be an aggregate type or tuple-like type");
  if constexpr (std::is_aggregate_v<T>) {
    auto fields = boost::pfr::structure_to_tuple(row);
    std::apply(
        [this](const auto&... args) { this->UpdateParamsBindings(args...); },
        fields);
  } else {
    return std::apply(
        [this](const auto&... args) { this->UpdateParamsBindings(args...); },
        row);
  }
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
