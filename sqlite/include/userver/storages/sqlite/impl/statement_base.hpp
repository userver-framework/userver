#pragma once

#include <cstdint>
#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

class StatementBase {
 public:
  virtual ~StatementBase() = default;

  // Bind methods
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
  virtual int ColumnCount() const noexcept = 0;
  virtual bool HasNext() const noexcept = 0;
  virtual bool IsDone() const noexcept = 0;
  virtual void Next() = 0;

  // Extract result methods
  virtual int RowsAffected() const noexcept = 0;
  virtual int LastInsertRowId() const noexcept = 0;
  virtual void Extract(int column, int8_t& val) const noexcept = 0;
  virtual void Extract(int column, uint8_t& val) const noexcept = 0;
  virtual void Extract(int column, int16_t& val) const noexcept = 0;
  virtual void Extract(int column, uint16_t& val) const noexcept = 0;
  virtual void Extract(int column, int32_t& val) const noexcept = 0;
  virtual void Extract(int column, uint32_t& val) const noexcept = 0;
  virtual void Extract(int column, int64_t& val) const noexcept = 0;
  virtual void Extract(int column, uint64_t& val) const noexcept = 0;
  virtual void Extract(int column, float& val) const noexcept = 0;
  virtual void Extract(int column, double& val) const noexcept = 0;
  virtual void Extract(int column, std::string& val) const noexcept = 0;
  virtual void Extract(int column,
                       std::vector<uint8_t>& val) const noexcept = 0;
};

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
