#pragma once

#include <storages/postgres/postgres_fwd.hpp>

#include <storages/postgres/result_set.hpp>

#include <postgresql/libpq-fe.h>
#include <memory>

namespace storages {
namespace postgres {
namespace detail {

/// @brief Wrapper for PGresult
class ResultSetImpl {
 public:
  using ResultHandle = std::unique_ptr<PGresult, decltype(&PQclear)>;

 public:
  ResultSetImpl(ResultHandle&& res) : handle_{std::move(res)} {};

  std::size_t RowCount() const;
  std::size_t FieldCount() const;

  std::size_t IndexOfName(std::string const& name) const;

  io::DataFormat GetFieldFormat(std::size_t col) const;
  bool IsFieldNull(std::size_t row, std::size_t col) const;
  std::size_t GetFieldLength(std::size_t row, std::size_t col) const;
  io::FieldBuffer GetFieldBuffer(std::size_t row, std::size_t col) const;

 private:
  ResultHandle handle_;
};

}  // namespace detail
}  // namespace postgres
}  // namespace storages
