#pragma once

#include <initializer_list>
#include <limits>
#include <memory>
#include <tuple>
#include <utility>

#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/io/boost_multiprecision.hpp>
#include <storages/postgres/io/chrono.hpp>
#include <storages/postgres/io/floating_point_types.hpp>
#include <storages/postgres/io/nullable_traits.hpp>
#include <storages/postgres/io/stream_text_parser.hpp>
#include <storages/postgres/io/string_types.hpp>
#include <storages/postgres/io/traits.hpp>
#include <storages/postgres/postgres_fwd.hpp>

#include <storages/postgres/detail/const_data_iterator.hpp>

#include <utils/demangle.hpp>

namespace storages {
namespace postgres {

struct FieldDescription {
  /// Index of the field in the result set
  std::size_t index;
  /// @brief The object ID of the field's data type.
  Oid type_oid;
  /// @brief The field name.
  // TODO string_view
  std::string name;
  /// @brief Data format of the field (text/binary).
  io::DataFormat data_format;
  /// @brief If the field can be identified as a column of a specific table,
  /// the object ID of the table; otherwise zero.
  Oid table_oid;
  /// @brief If the field can be identified as a column of a specific table,
  /// the attribute number of the column; otherwise zero.
  Integer table_column;
  /// @brief The data type size (see pg_type.typlen). Note that negative
  /// values denote variable-width types.
  Integer type_size;
  /// @brief The type modifier (see pg_attribute.atttypmod). The meaning of
  /// the modifier is type-specific.
  Integer type_modifier;
};

/// @brief A wrapper for PGresult to access field descriptions.
class RowDescription {
 public:
  RowDescription(detail::ResultWrapperPtr res) : res_{res} {}
  // TODO pass a container for connection-specific user types
  bool CanBeReadInBinary(/* user_types */) const;
  // TODO pass a container for connection-specific user types
  io::DataFormat BestReplyFormat(/* user_types */) const;
  // TODO interface for iterating field descriptions
 private:
  detail::ResultWrapperPtr res_;
};

class Row;
class ResultSet;
template <typename T>
class TypedResultSet;

/// @brief Accessor to a single field in a result set's row
class Field {
 public:
  using size_type = std::size_t;

 public:
  size_type RowIndex() const { return row_index_; }
  size_type FieldIndex() const { return field_index_; }

  //@{
  /** @name Field metadata */
  /// Field name as named in query
  /// TODO Replace with string_view
  std::string Name() const;
  FieldDescription Description() const;

  io::DataFormat GetDataFormat() const;
  Oid GetTypeOid() const;
  //@}

  //@{
  /** @name Data access */
  bool IsNull() const;

  template <typename T>
  size_type To(T&& val) const {
    using ValueType = typename std::decay<T>::type;
    auto fb = GetBuffer();
    return ReadNullable(fb, std::forward<T>(val),
                        io::traits::IsNullable<ValueType>{});
  }

  template <typename T>
  void Coalesce(T& val, const T& default_val) const {
    if (!IsNull())
      To(val);
    else
      val = default_val;
  }

  template <typename T>
  typename std::decay<T>::type As() const {
    T val;
    To(val);
    return val;
  }

  template <typename T>
  typename std::decay<T>::type Coalesce(const T& default_val) const {
    if (IsNull()) return default_val;
    return As<T>();
  }
  //@}
 private:
  io::FieldBuffer GetBuffer() const;

 protected:
  friend class Row;

  Field(detail::ResultWrapperPtr res, size_type row, size_type col)
      : res_{res}, row_index_{row}, field_index_{col} {}

  template <typename T>
  size_type ReadNullable(const io::FieldBuffer& fb, T&& val,
                         std::true_type) const {
    using ValueType = typename std::decay<T>::type;
    using NullSetter = io::traits::GetSetNull<ValueType>;
    if (fb.is_null) {
      NullSetter::SetNull(val);
    } else {
      Read(fb, std::forward<T>(val));
    }
    return fb.length;
  }
  template <typename T>
  size_type ReadNullable(const io::FieldBuffer& fb, T&& val,
                         std::false_type) const {
    if (fb.is_null) {
      throw FieldValueIsNull{field_index_};
    } else {
      Read(fb, std::forward<T>(val));
    }
    return fb.length;
  }

  template <typename T>
  void Read(const io::FieldBuffer& fb, T&& val) const {
    // TODO Static assert that the type has a parser at all
    using ValueType = typename std::decay<T>::type;
    if (fb.format == io::DataFormat::kTextDataFormat) {
      ReadText(
          fb, std::forward<T>(val),
          io::traits::HasParser<ValueType, io::DataFormat::kTextDataFormat>{});
    } else {
      ReadBinary(fb, std::forward<T>(val),
                 io::traits::HasParser<ValueType,
                                       io::DataFormat::kBinaryDataFormat>{});
    }
  }

  template <typename T>
  void ReadText(io::FieldBuffer const& fb, T&& val, std::true_type) const {
    using ValueType = typename std::decay<T>::type;
    using Parser =
        typename io::traits::IO<ValueType,
                                io::DataFormat::kTextDataFormat>::ParserType;
    Parser{std::forward<T>(val)}(fb);
  }
  template <typename T>
  void ReadText(io::FieldBuffer const&, T&&, std::false_type) const {
    throw NoValueParser{::utils::GetTypeName<T>(),
                        io::DataFormat::kTextDataFormat};
  }

  template <typename T>
  void ReadBinary(io::FieldBuffer const& fb, T&& val, std::true_type) const {
    using ValueType = typename std::decay<T>::type;
    using Parser =
        typename io::traits::IO<ValueType,
                                io::DataFormat::kBinaryDataFormat>::ParserType;
    Parser{std::forward<T>(val)}(fb);
  }
  template <typename T>
  void ReadBinary(io::FieldBuffer const&, T&&, std::false_type) const {
    throw NoValueParser{::utils::GetTypeName<T>(),
                        io::DataFormat::kBinaryDataFormat};
  }
  //@{
  /** @name Iteration support */
  bool IsValid() const;
  int Compare(const Field& rhs) const;
  std::ptrdiff_t Distance(const Field& rhs) const;
  Field& Advance(std::ptrdiff_t);
  //@}

  detail::ResultWrapperPtr res_;
  size_type row_index_;
  size_type field_index_;
};

/// @brief Iterator over fields in a result set's row
class ConstFieldIterator
    : public detail::ConstDataIterator<ConstFieldIterator, Field> {
  friend class Row;

  ConstFieldIterator(detail::ResultWrapperPtr res, size_type row, size_type col)
      : ConstDataIterator(res, row, col) {}
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
  size_type RowIndex() const { return row_index_; }

  RowDescription GetDescription() const { return {res_}; }
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
  /// Checked field access by index.
  /// @throws FieldIndexOutOfBounds if index is out of bounds
  reference At(size_type index) const;
  /// Access field by it's name
  /// @throws FieldNameDoesntExist if the result set doesn't contain
  ///         such a field
  reference operator[](const std::string& name) const;
  //@}

  //@{
  /** @name Bulk access to row's data */
  /// Read fields into variables in order of their appearance in the row
  template <typename... T>
  void To(T&&... val) const;
  template <typename... T>
  void To(std::tuple<T...>&) const;

  template <typename... T>
  std::tuple<T...> As() const {
    std::tuple<T...> res;
    To(res);
    return res;
  }
  template <typename T>
  T AsTuple() const {
    T res;
    To(res);
    return res;
  }

  /// Read fields into variables in order of their names in the first argument
  template <typename... T>
  void To(const std::initializer_list<std::string>& names, T&&... val) const;
  template <typename... T>
  void To(const std::initializer_list<std::string>& names,
          std::tuple<T...>& val) const;
  template <typename... T>
  std::tuple<T...> As(const std::initializer_list<std::string>& names) const {
    std::tuple<T...> res;
    To(names, res);
    return res;
  }

  /// Read fields into variables in order of their indexes in the first
  /// argument
  template <typename... T>
  void To(const std::initializer_list<size_type>& indexes, T&&... val) const;
  template <typename... T>
  void To(const std::initializer_list<size_type>& indexes,
          std::tuple<T...>& val) const;
  template <typename... T>
  std::tuple<T...> As(const std::initializer_list<size_type>& indexes) const {
    std::tuple<T...> res;
    To(indexes, res);
    return res;
  }
  //@}

  size_type IndexOfName(std::string const&) const;

 protected:
  friend class ResultSet;

  Row(detail::ResultWrapperPtr res, size_type row)
      : res_{res}, row_index_{row} {}

  //@{
  /** @name Iteration support */
  bool IsValid() const;
  int Compare(const Row& rhs) const;
  std::ptrdiff_t Distance(const Row& rhs) const;
  Row& Advance(std::ptrdiff_t);
  //@}

  detail::ResultWrapperPtr res_;
  size_type row_index_;
};

/// @name Iterator over rows in a result set
class ConstRowIterator
    : public detail::ConstDataIterator<ConstRowIterator, Row> {
  friend class ResultSet;

  ConstRowIterator(detail::ResultWrapperPtr res, size_type row)
      : ConstDataIterator(res, row) {}
};

/// @brief PostgreSQL result set
///
/// Provides random access to rows via indexing operations
/// and bidirectional iteration via iterators.
///
/// ## Usage synopsis
/// ```
/// auto trx = ...;
/// auto res = trx.Execute("select a, b from table");
/// for (auto row : res) {
///   // Process row data
/// }
/// ```
class ResultSet {
 public:
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  static constexpr size_type npos = std::numeric_limits<size_type>::max();

  //@{
  /** @name Row container concept */
  using const_iterator = ConstRowIterator;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  using value_type = Row;
  using reference = const value_type;
  using pointer = const_iterator;
  //@}

 public:
  explicit ResultSet(detail::ResultWrapperPtr pimpl) : pimpl_{pimpl} {}

  /// Number of rows in the result set
  size_type Size() const;
  bool IsEmpty() const { return Size() == 0; }
  explicit operator bool() const { return !IsEmpty(); }
  bool operator!() const { return IsEmpty(); }

  //@{
  /** @name Row container interface */
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

  // TODO Check resultset is not empty
  reference Front() const;
  reference Back() const;

  //@{
  /** @name Random access interface */
  /// Access a row by index
  /// Accessing a row beyond the result set size is undefined behaviour
  reference operator[](size_type index) const;
  /// Range-checked access to a row by index
  /// Accessing a row beyond the result set size will throw an exception
  /// @throws RowIndexOutOfBounds
  reference At(size_type index) const;
  //@}

  //@}

  //@{
  /** @name ResultSet metadata access */
  // TODO ResultSet metadata access interface
  size_type FieldCount() const;
  RowDescription GetRowDescription() const { return {pimpl_}; }
  //@}

  //@{
  /** @name Typed results */
  /// @brief Get a wrapper for iterating over a set of typed results.
  /// For more information see @ref psql_typed_results
  template <typename T>
  TypedResultSet<T> As() const {
    return TypedResultSet<T>{*this};
  }

  /// @brief Extract data into a container.
  /// For more information see @ref psql_typed_results
  template <typename Container>
  Container AsContainer() const;
  //@}
 private:
  template <typename T>
  friend class TypedResultSet;

  detail::ResultWrapperPtr pimpl_;
};

namespace detail {

//@{
/** @name Sequental field extraction */
template <typename IndexTuple, typename... T>
struct RowDataExtractorBase;

template <std::size_t... Indexes, typename... T>
struct RowDataExtractorBase<std::index_sequence<Indexes...>, T...> {
  static void ExtractValues(const Row& row, T&&... val) {
    (row[Indexes].To(std::forward<T>(val)), ...);
  }
  static void ExtractTuple(const Row& row, std::tuple<T...>& val) {
    std::tuple<T...> tmp{row[Indexes].template As<T>()...};
    tmp.swap(val);
  }

  static void ExtractValues(const Row& row,
                            const std::initializer_list<std::string>& names,
                            T&&... val) {
    (row[*(names.begin() + Indexes)].To(std::forward<T>(val)), ...);
  }
  static void ExtractTuple(const Row& row,
                           const std::initializer_list<std::string>& names,
                           std::tuple<T...>& val) {
    std::tuple<T...> tmp{row[*(names.begin() + Indexes)].template As<T>()...};
    tmp.swap(val);
  }

  static void ExtractValues(const Row& row,
                            const std::initializer_list<std::size_t>& indexes,
                            T&&... val) {
    (row[*(indexes.begin() + Indexes)].To(std::forward<T>(val)), ...);
  }
  static void ExtractTuple(const Row& row,
                           const std::initializer_list<std::size_t>& indexes,
                           std::tuple<T...>& val) {
    std::tuple<T...> tmp{row[*(indexes.begin() + Indexes)].template As<T>()...};
    tmp.swap(val);
  }
};

template <typename... T>
struct RowDataExtractor
    : RowDataExtractorBase<std::index_sequence_for<T...>, T...> {};
//@}

}  // namespace detail

template <typename... T>
void Row::To(T&&... val) const {
  if (sizeof...(T) > Size()) {
    throw InvalidTupleSizeRequested(Size(), sizeof...(T));
  }
  detail::RowDataExtractor<T...>::ExtractValues(*this, std::forward<T>(val)...);
}

template <typename... T>
void Row::To(std::tuple<T...>& val) const {
  if (sizeof...(T) > Size()) {
    throw InvalidTupleSizeRequested(Size(), sizeof...(T));
  }
  detail::RowDataExtractor<T...>::ExtractTuple(*this, val);
}

template <typename... T>
void Row::To(const std::initializer_list<std::string>& names,
             T&&... val) const {
  if (sizeof...(T) != names.size()) {
    throw FieldTupleMismatch(names.size(), sizeof...(T));
  }
  detail::RowDataExtractor<T...>::ExtractValues(*this, names,
                                                std::forward<T>(val)...);
}

template <typename... T>
void Row::To(const std::initializer_list<std::string>& names,
             std::tuple<T...>& val) const {
  if (sizeof...(T) != names.size()) {
    throw FieldTupleMismatch(names.size(), sizeof...(T));
  }
  detail::RowDataExtractor<T...>::ExtractTuple(*this, names, val);
}

template <typename... T>
void Row::To(const std::initializer_list<size_type>& indexes,
             T&&... val) const {
  if (sizeof...(T) != indexes.size()) {
    throw FieldTupleMismatch(indexes.size(), sizeof...(T));
  }
  detail::RowDataExtractor<T...>::ExtractValues(*this, indexes,
                                                std::forward<T>(val)...);
}

template <typename... T>
void Row::To(const std::initializer_list<size_type>& indexes,
             std::tuple<T...>& val) const {
  if (sizeof...(T) != indexes.size()) {
    throw FieldTupleMismatch(indexes.size(), sizeof...(T));
  }
  detail::RowDataExtractor<T...>::ExtractTuple(*this, indexes, val);
}

namespace detail {

template <typename T, typename = ::utils::void_t<>>
struct CanReserve : std::false_type {};

template <typename T>
struct CanReserve<T, ::utils::void_t<decltype(std::declval<T>().reserve(
                         std::declval<std::size_t>()))>> : std::true_type {};

template <typename T>
constexpr bool kCanReserve = CanReserve<T>::value;

template <typename T, typename = ::utils::void_t<>>
struct CanPushBack : std::false_type {};

template <typename T>
struct CanPushBack<T, ::utils::void_t<decltype(std::declval<T>().push_back(
                          std::declval<typename T::value_type>()))>>
    : std::true_type {};

template <typename T>
constexpr bool kCanPushBack = CanPushBack<T>::value;

template <typename T>
auto Inserter(T& container) {
  if constexpr (kCanPushBack<T>) {
    return std::back_inserter(container);
  } else {
    return std::inserter(container, container.end());
  }
}

}  // namespace detail

template <typename Container>
Container ResultSet::AsContainer() const {
  using ValueType = typename Container::value_type;
  Container c;
  if constexpr (detail::kCanReserve<Container>) {
    c.reserve(Size());
  }
  auto res = As<ValueType>();
  std::copy(res.begin(), res.end(), detail::Inserter(c));
  return c;
}

}  // namespace postgres
}  // namespace storages

#include <storages/postgres/typed_result_set.hpp>
