#include "result_set_impl.hpp"

namespace storages {
namespace postgres {
namespace detail {

std::size_t ResultSetImpl::RowCount() const { return PQntuples(handle_.get()); }

std::size_t ResultSetImpl::FieldCount() const {
  return PQnfields(handle_.get());
}

std::size_t ResultSetImpl::IndexOfName(const std::string& name) const {
  auto n = PQfnumber(handle_.get(), name.c_str());
  if (n < 0) return ResultSet::npos;
  return n;
}

bool ResultSetImpl::IsFieldNull(std::size_t row, std::size_t col) const {
  return PQgetisnull(handle_.get(), row, col);
}

io::DataFormat ResultSetImpl::GetFieldFormat(std::size_t col) const {
  return static_cast<io::DataFormat>(PQfformat(handle_.get(), col));
}

std::size_t ResultSetImpl::GetFieldLength(std::size_t row,
                                          std::size_t col) const {
  return PQgetlength(handle_.get(), row, col);
}

io::FieldBuffer ResultSetImpl::GetFieldBuffer(std::size_t row,
                                              std::size_t col) const {
  return io::FieldBuffer{IsFieldNull(row, col), GetFieldFormat(col),
                         GetFieldLength(row, col),
                         PQgetvalue(handle_.get(), row, col)};
}

}  // namespace detail
}  // namespace postgres
}  // namespace storages
