#pragma once

/// @file userver/storages/sqlite/result_set.hpp
/// @brief Result accessors

#include <cstddef>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

class ResultSet {
 public:
  using size_type = std::size_t;

  ResultSet();

  size_type Size() const;
  bool IsEmpty() const { return Size() == 0; }

  size_type RowsAffected() const;

  template <typename T>
  auto AsSingleRow() const;
};

template <typename T>
auto ResultSet::AsSingleRow() const {
  return T{};
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
