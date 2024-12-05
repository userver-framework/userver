#pragma once

/// @file userver/storages/sqlite/result_set.hpp
/// @brief Result accessors

#include <cstddef>
#include <limits>
#include <optional>
#include <vector>

#include <userver/storages/sqlite/row_types.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

class Row {
 public:
  Row() = default;

 private:
  friend class ResultSet;
};

class ConstRowIterator {
 public:
  ConstRowIterator() = default;

 private:
  friend class ResultSet;
};

class ReverseConstRowIterator {
 public:
  ReverseConstRowIterator() = default;

 private:
  friend class ResultSet;
};

class ResultSet {
 public:
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  static constexpr size_type npos = std::numeric_limits<size_type>::max();

  using const_iterator = ConstRowIterator;
  using const_reverse_iterator = ReverseConstRowIterator;

  using value_type = Row;
  using reference = value_type;
  using pointer = const_iterator;

  ResultSet();

  size_type Size() const;
  bool IsEmpty() const { return Size() == 0; }

  size_type RowsAffected() const;

  const_iterator cbegin() const&;
  const_iterator begin() const& { return cbegin(); }
  const_iterator cend() const&;
  const_iterator end() const& { return cend(); }

  const_iterator cbegin() const&& = delete;
  const_iterator begin() const&& = delete;
  const_iterator cend() const&& = delete;
  const_iterator end() const&& = delete;

  const_reverse_iterator crbegin() const&;
  const_reverse_iterator rbegin() const& { return crbegin(); }
  const_reverse_iterator crend() const&;
  const_reverse_iterator rend() const& { return crend(); }

  const_reverse_iterator crbegin() const&& = delete;
  const_reverse_iterator rbegin() const&& = delete;
  const_reverse_iterator crend() const&& = delete;
  const_reverse_iterator rend() const&& = delete;

  reference Front() const&;
  reference Back() const&;

  reference Front() const&& = delete;
  reference Back() const&& = delete;

  reference operator[](size_type index) const&;
  reference operator[](size_type index) const&& = delete;

  template <typename T>
  std::vector<T> AsVector() const;
  template <typename T>
  std::vector<T> AsVector(RowTag) const;
  template <typename T>
  std::vector<T> AsVector(FieldTag) const;

  template <typename Container>
  Container AsContainer() const;
  template <typename Container>
  Container AsContainer(RowTag) const;
  template <typename Container>
  Container AsContainer(FieldTag) const;

  template <typename T>
  auto AsSingleRow() const;
  template <typename T>
  auto AsSingleRow(RowTag) const;
  template <typename T>
  auto AsSingleRow(FieldTag) const;

  template <typename T>
  std::optional<T> AsOptionalSingleRow() const;
  template <typename T>
  std::optional<T> AsOptionalSingleRow(RowTag) const;
  template <typename T>
  std::optional<T> AsOptionalSingleRow(FieldTag) const;
};

template <typename T>
std::vector<T> ResultSet::AsVector() const {
  return std::move(*this).AsContainer<std::vector<T>>();
}

template <typename T>
std::vector<T> ResultSet::AsVector(RowTag) const {
  return std::move(*this).AsContainer<std::vector<T>>(kRowTag);
}

template <typename T>
std::vector<T> ResultSet::AsVector(FieldTag) const {
  return std::move(*this).AsContainer<std::vector<T>>(kFieldTag);
}

template <typename Container>
Container ResultSet::AsContainer() const {
  return AsContainer<Container>(kRowTag);
};

template <typename Container>
Container ResultSet::AsContainer(RowTag) const {
  return Container{};
}

template <typename Container>
Container ResultSet::AsContainer(FieldTag) const {
  return Container{};
}

template <typename T>
auto ResultSet::AsSingleRow() const {
  return AsSingleRow<T>(kFieldTag);
}

template <typename T>
auto ResultSet::AsSingleRow(RowTag) const {
  return T{};
}

template <typename T>
auto ResultSet::AsSingleRow(FieldTag) const {
  return T{};
}

template <typename T>
std::optional<T> ResultSet::AsOptionalSingleRow() const {
  return IsEmpty() ? std::nullopt : std::optional<T>{AsSingleRow<T>()};
}

template <typename T>
std::optional<T> ResultSet::AsOptionalSingleRow(RowTag) const {
  return IsEmpty() ? std::nullopt : std::optional<T>{AsSingleRow<T>(kRowTag)};
}

template <typename T>
std::optional<T> ResultSet::AsOptionalSingleRow(FieldTag) const {
  return IsEmpty() ? std::nullopt : std::optional<T>{AsSingleRow<T>(kFieldTag)};
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
