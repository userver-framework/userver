#pragma once

#include <iterator>
#include <optional>
#include <string>

#include <boost/pfr/core.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/fast_pimpl.hpp>

#include <userver/storages/mysql/impl/io/parse.hpp>
#include <userver/storages/mysql/impl/io/traits.hpp>

USERVER_NAMESPACE_BEGIN

// TODO : maybe drop? or just drop parsing and leave as vector<vector<string>>

namespace storages::mysql {

namespace impl {
class MySQLResult;
}

template <typename T>
class TypedResultSet;

class ResultSet final {
 public:
  explicit ResultSet(impl::MySQLResult&& mysql_result);
  ~ResultSet();

  ResultSet(const ResultSet& other) = delete;
  ResultSet(ResultSet&& other) noexcept;

  std::size_t RowsCount() const;
  std::size_t FieldsCount() const;

  bool Empty() const;

  template <typename T>
  TypedResultSet<T> AsRows() &&;

  template <typename T>
  T AsSingleRow() &&;

  template <typename T>
  std::optional<T> AsOptionalSingleRow() &&;

  template <typename Container>
  Container AsContainer() &&;

 private:
  template <typename T>
  friend class TypedResultSet;

  std::string& GetAt(std::size_t row, std::size_t column);

  class Impl;
  utils::FastPimpl<Impl, 24, 8> impl_;
};

template <typename T>
TypedResultSet<T> ResultSet::AsRows() && {
  if (!Empty()) {
    UINVARIANT(boost::pfr::tuple_size_v<T> == FieldsCount(),
               "Fields count mismatch");
  }

  return TypedResultSet<T>{std::move(*this)};
}

template <typename T>
T ResultSet::AsSingleRow() && {
  if (RowsCount() != 1) {
    throw std::runtime_error{"Oh well"};
  }

  return *(std::move(*this).AsRows<T>().begin());
}

template <typename T>
std::optional<T> ResultSet::AsOptionalSingleRow() && {
  if (RowsCount() > 1) {
    throw std::runtime_error{"Oh well"};
  }

  if (Empty()) {
    return std::nullopt;
  } else {
    return AsSingleRow<T>();
  }
}

template <typename Container>
Container ResultSet::AsContainer() && {
  static_assert(impl::io::kIsRange<Container>,
                "The type isn't actually a container");
  using Row = typename Container::value_type;

  Container result;
  if (impl::io::kIsReservable<Container>) {
    result.reserve(RowsCount());
  }

  auto rows = std::move(*this).AsRows<Row>();
  std::move(rows.begin(), rows.end(), impl::io::Inserter(result));
  return result;
}

template <typename T>
class TypedResultSet final {
 public:
  explicit TypedResultSet(ResultSet&& result_set)
      : result_set_{std::move(result_set)} {}
  ~TypedResultSet() = default;

  class Iterator final {
   public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using reference = value_type&;
    using pointer = value_type*;

    Iterator() = default;
    Iterator(ResultSet& result_set, std::size_t ind)
        : result_set_{&result_set}, ind_{ind} {}

    Iterator operator++(int);
    Iterator& operator++();
    reference operator*() const;
    pointer operator->() const;

    bool operator==(const Iterator& other) const;
    bool operator!=(const Iterator& other) const;

   private:
    T& UpdateValue() const;

    class FieldMapper final {
     public:
      FieldMapper(ResultSet* result_set, std::size_t row_ind)
          : result_set_{result_set}, row_ind_{row_ind} {}

      template <typename Field, size_t Index>
      void operator()(Field& field,
                      std::integral_constant<std::size_t, Index> i) const {
        impl::io::GetParser(field)(result_set_->GetAt(row_ind_, i));
      }

     private:
      ResultSet* result_set_;
      std::size_t row_ind_;
    };

    ResultSet* result_set_{nullptr};
    std::size_t ind_{0};
    mutable std::optional<value_type> current_value_{std::nullopt};
  };

  auto begin() { return Iterator{result_set_, 0}; }
  auto end() { return Iterator{result_set_, result_set_.RowsCount()}; }

 private:
  ResultSet result_set_;
};

template <typename T>
typename TypedResultSet<T>::Iterator TypedResultSet<T>::Iterator::operator++(
    int) {
  Iterator old{*result_set_, ind_};
  old.current_value_ = std::move_if_noexcept(current_value_);

  ++(*this);
  return old;
}

template <typename T>
typename TypedResultSet<T>::Iterator&
TypedResultSet<T>::Iterator::operator++() {
  ++ind_;
  current_value_.reset();

  return *this;
}

template <typename T>
typename TypedResultSet<T>::Iterator::reference
TypedResultSet<T>::Iterator::operator*() const {
  return UpdateValue();
}

template <typename T>
typename TypedResultSet<T>::Iterator::pointer
TypedResultSet<T>::Iterator::operator->() const {
  return &UpdateValue();
}

template <typename T>
bool TypedResultSet<T>::Iterator::operator==(const Iterator& other) const {
  return result_set_ == other.result_set_ && ind_ == other.ind_;
}

template <typename T>
bool TypedResultSet<T>::Iterator::operator!=(const Iterator& other) const {
  return !(*this == other);
}

template <typename T>
T& TypedResultSet<T>::Iterator::UpdateValue() const {
  if (!current_value_.has_value()) {
    T row{};
    boost::pfr::for_each_field(row, FieldMapper{result_set_, ind_});
    current_value_.emplace(std::move(row));
  }
  return *current_value_;
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
