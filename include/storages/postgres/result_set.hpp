#pragma once

#include <initializer_list>
#include <memory>
#include <storages/postgres/detail/const_data_iterator.hpp>

#include <utils/string_view.hpp>

namespace storages {
namespace postgres {

class FieldDescription;

namespace detail {

class ResultSetImpl;
using ResultSetImplPtr = std::shared_ptr<ResultSetImpl>;

}  // namespace detail

/// @brief Accessor to a single field in a result set's row
class Field {
 public:
  using size_type = std::size_t;

 public:
  size_type RowIndex() const;
  size_type FieldIndex() const;

  //@{
  /** @name Field metadata */
  utils::string_view Name() const;
  FieldDescription Description() const;
  //@}

  //@{
  /** @name Data access */
  bool IsNull() const;

  template <typename T>
  void To(T& val) const;

  template <typename T>
  void Coalesce(T& val, const T& default_val) const {
    if (!IsNull())
      To(val);
    else
      val = default_val;
  }

  template <typename T>
  typename std::decay<T>::type As() const;

  template <typename T>
  typename std::decay<T>::type Coalesce(const T& default_val) const;
  //@}
};

/// @brief Iterator over fields in a result set's row
class ConstFieldIterator
    : public detail::ConstDataIterator<ConstFieldIterator, Field> {
 public:
  ConstFieldIterator& Advance(difference_type);
  int Compare(const ConstFieldIterator& rhs) const;
  difference_type Distance(const ConstFieldIterator& rhs) const;
  bool IsValid() const;
};

/// Data row in a result set
/// This class is a mere accessor to underlying result set data buffer,
/// must not be used outside of result set life scope.
///
/// Mimics field container
class Row {
 public:
  //@{
  /** @name Field container concept */
  using size_type = std::size_t;
  using const_iterator = ConstFieldIterator;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  using value_type = Field;
  using reference = Field;
  using pointer = const_iterator;
  //@}
 public:
  size_type RowIndex() const;
  //@{
  /** @name Field container interface */
  /// Number of fields
  size_type Size() const;

  //@{
  /** @name Forward iteration */
  const_iterator cbegin() const;
  const_iterator begin() const { return cbegin(); }
  const_iterator cend() const;
  const_iterator end() const { return cend(); }
  //@}
  //@{
  /** @name Reverse iteration */
  const_reverse_iterator crbegin() const;
  const_reverse_iterator rbegin() const { return crbegin(); }
  const_reverse_iterator crend() const;
  const_reverse_iterator rend() const { return crend(); }
  //@}

  /// Access field by index
  /// Using a field index outside of bounds is undefined behaviour
  reference operator[](size_type index) const;
  /// Access field by it's name
  /// Will throw @exception NoSuchField if the result set doesn't contain
  /// such a field
  reference operator[](const utils::string_view& name) const;
  //@}

  //@{
  /** @name Bulk access to row's data */
  /// Read fields into variables in order of their appearance in the row
  template <typename... T>
  void To(T&... val) const;

  /// Read fields into variables in order of their names in the first argument
  template <typename... T>
  void To(const std::initializer_list<utils::string_view>& names,
          T&... val) const;

  /// Read fields into variables in order of their indexes in the first
  /// argument
  template <typename... T>
  void To(const std::initializer_list<size_type>& indexes, T&... val) const;
  //@}
};

/// @name Iterator over rows in a result set
class ConstRowIterator
    : public detail::ConstDataIterator<ConstRowIterator, Row> {
 public:
  ConstRowIterator& Advance(difference_type);
  int Compare(const ConstRowIterator& rhs) const;
  difference_type Distance(const ConstRowIterator& rhs) const;
  bool IsValid() const;
};

/// @brief PostgreSQL result set
///
/// Provides random access to rows via indexing operations
/// and bidirectional iteration via iterators.
///
/// ## Usage synopsis
/// ```
/// auto res = trx.Execute("select a, b from table");
/// for (auto row : res) {
///   // Process row data
/// }
/// ```
class ResultSet {
 public:
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  //@{
  /** @name Row container concept */
  using const_iterator = ConstRowIterator;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  using value_type = Row;
  using reference = const value_type;
  using pointer = const_iterator;
  //@}

 public:
  ResultSet(detail::ResultSetImplPtr pimpl) : pimpl_{pimpl} {}

  //@{
  /** @name Row container interface */
  /// Number of rows in the result set
  size_type Size() const;
  bool IsEmpty() const;

  //@{
  /** @name Forward iteration */
  const_iterator cbegin() const;
  const_iterator begin() const { return cbegin(); }
  const_iterator cend() const;
  const_iterator end() const { return cend(); }
  //@}
  //@{
  /** @name Reverse iteration */
  const_reverse_iterator crbegin() const;
  const_reverse_iterator rbegin() const { return crbegin(); }
  const_reverse_iterator crend() const;
  const_reverse_iterator rend() const { return crend(); }
  //@}

  reference Front() const;
  reference Back() const;

  //@{
  /** @name Random access interface */
  /// Access a row by index
  /// Accessing a row beyond the result set size is undefined behaviour
  reference operator[](size_type index) const;
  /// Range-checked access to a row by index
  /// Accessing a row beyond the result set size will throw an exception
  reference At(size_type index) const;
  //@}

  //@}

  //@{
  /** @name ResultSet metadata access */
  // TODO ResultSet metadata access interface
  //@}

 private:
  detail::ResultSetImplPtr pimpl_;
};

}  // namespace postgres
}  // namespace storages
