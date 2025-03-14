#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <boost/pfr/core.hpp>
#include <boost/pfr/tuple_size.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

class StatementBinder {
  virtual void Bind(const int index, const int32_t value) = 0;
  virtual void Bind(const int index, const int64_t value) = 0;
  virtual void Bind(const int index, const uint32_t value) = 0;
  virtual void Bind(const int index, const uint64_t value) = 0;
  virtual void Bind(const int index, const double value) = 0;
  virtual void Bind(const int index, const std::string& value) = 0;
  virtual void Bind(const int index, const std::string_view value) = 0;
  virtual void Bind(const int index, const char* value, const int size) = 0;
  virtual void Bind(const int index) = 0;
};

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
