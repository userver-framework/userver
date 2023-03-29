#pragma once

/// @file userver/storages/postgres/result_set.hpp
/// @brief Result accessors

#include <initializer_list>
#include <limits>
#include <memory>
#include <tuple>
#include <utility>

#include <fmt/format.h>

#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/supported_types.hpp>
#include <userver/storages/postgres/postgres_fwd.hpp>

#include <userver/storages/postgres/detail/const_data_iterator.hpp>

#include <userver/compiler/demangle.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

/// @page pg_process_results uPg: Working with result sets
///
/// A result set returned from Execute function is a thin read only wrapper
/// around the libpq result. It can be copied around as it contains only a
/// smart pointer to the underlying result set.
///
/// The result set's lifetime is not limited by the transaction in which it was
/// created. In can be used after the transaction is committed or rolled back.
///
/// @par Iterating result set's rows
///
/// The ResultSet provides interface for range-based iteration over its rows.
/// @code
/// auto result = trx.Execute("select foo, bar from foobar");
/// for (auto row : result) {
///   // Process row data here
/// }
/// @endcode
///
/// Also rows can be accessed via indexing operators.
/// @code
/// auto result = trx.Execute("select foo, bar from foobar");
/// for (auto idx = 0; idx < result.Size(); ++idx) {
///   auto row = result[idx];
///   // process row data here
/// }
/// @endcode
///
/// @par Accessing fields in a row
///
/// Fields in a row can be accessed by their index, by field name and can be
/// iterated over. Invalid index or name will throw an exception.
/// @code
/// auto f1 = row[0];
/// auto f2 = row["foo"];
/// auto f3 = row[1];
/// auto f4 = row["bar"];
///
/// for (auto f : row) {
///   // Process field here
/// }
/// @endcode
///
/// @par Extracting field's data to variables
///
/// A Field object provides an interface to convert underlying buffer to a
/// C++ variable of supported type. Please see @ref pg_types for more
/// information on supported types.
///
/// Functions Field::As and Field::To can throw an exception if the field
/// value is `null`. Their Field::Coalesce counterparts instead set the result
/// to default value.
///
/// All data extraction functions can throw parsing errors (descendants of
/// ResultSetError).
///
/// @code
/// auto foo = row["foo"].As<int>();
/// auto bar = row["bar"].As<std::string>();
///
/// foo = row["foo"].Coalesce(42);
/// // There is no parser for char*, so a string object must be passed here.
/// bar = row["bar"].Coalesce(std::string{"bar"});
///
/// row["foo"].To(foo);
/// row["bar"].To(bar);
///
/// row["foo"].Coalesce(foo, 42);
/// // The type is deduced by the first argument, so the second will be also
/// // treated as std::string
/// row["bar"].Coalesce(bar, "baz");
/// @endcode
///
/// @par Extracting data directly from a Row object
///
/// Data can be extracted straight from a Row object to a pack or a tuple of
/// user variables. The number of user variables cannot exceed the number of
/// fields in the result. If it does, an exception will be thrown.
///
/// When used without additional parameters, the field values are extracted
/// in the order of their appearance.
///
/// When a subset of the fields is needed, the fields can be specified by their
/// indexes or names.
///
/// Row's data extraction functions throw exceptions as the field extraction
/// functions. Also a FieldIndexOutOfBounds or FieldNameDoesntExist can be
/// thrown.
///
/// Statements that return user-defined PostgreSQL type may be called as
/// returning either one-column row with the whole type in it or as multi-column
/// row with every column representing a field in the type. For the purpose of
/// disambiguation, kRowTag may be used.
///
/// When a first column is extracted, it is expected that the result set
/// contains the only column, otherwise an exception will be thrown.
///
/// @code
/// auto [foo, bar] = row.As<int, std::string>();
/// row.To(foo, bar);
///
/// auto [bar, foo] = row.As<std::string, int>({1, 0});
/// row.To({1, 0}, bar, foo);
///
/// auto [bar, foo] = row.As<std::string, int>({"bar", "foo"});
/// row.To({"bar", "foo"}, bar, foo);
///
/// // extract the whole row into a row-type structure.
/// // The FooBar type must not have the C++ to PostgreSQL mapping in this case
/// auto foobar = row.As<FooBar>();
/// row.To(foobar);
/// // If the FooBar type does have the mapping, the function call must be
/// // disambiguated.
/// foobar = row.As<FooBar>(kRowTag);
/// row.To(foobar, kRowTag);
/// @endcode
///
/// In the following example it is assumed that the row has a single column
/// and the FooBar type is mapped to a PostgreSQL type.
///
/// @note The row is used to extract different types, it doesn't mean it will
/// actually work with incompatible types.
///
/// @code
/// auto foobar = row.As<FooBar>();
/// row.To(foobar);
///
/// auto str = row.As<std::string>();
/// auto i = row.As<int>();
/// @endcode
///
///
/// @par Converting a Row to a user row type
///
/// A row can be converted to a user type (tuple, structure, class), for more
/// information on data type requrements see @ref pg_user_row_types
///
/// @todo Interface for converting rows to arbitrary user types
///
/// @par Converting ResultSet to a result set with user row types
///
/// A result set can be represented as a set of user row types or extracted to
/// a container. For more information see @ref pg_user_row_types
///
/// @todo Interface for copying a ResultSet to an output iterator.
///
/// @par Non-select query results
///
/// @todo Process non-select result and provide interface. Do the docs.
///
///
/// ----------
///
/// @htmlonly <div class="bottom-nav"> @endhtmlonly
/// ⇦ @ref pg_run_queries | @ref pg_types ⇨
/// @htmlonly </div> @endhtmlonly

struct FieldDescription {
  /// Index of the field in the result set
  std::size_t index;
  /// @brief The object ID of the field's data type.
  Oid type_oid;
  /// @brief The field name.
  // TODO string_view
  std::string name;
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
  RowDescription(detail::ResultWrapperPtr res) : res_{std::move(res)} {}

  /// Check that all fields can be read in binary format
  /// @throw NoBinaryParser if any of the fields doesn't have a binary parser
  void CheckBinaryFormat(const UserTypes& types) const;
  // TODO interface for iterating field descriptions
 private:
  detail::ResultWrapperPtr res_;
};

class Row;
class ResultSet;
template <typename T, typename ExtractionTag>
class TypedResultSet;

/// @brief Accessor to a single field in a result set's row
class Field {
 public:
  using size_type = std::size_t;

  size_type RowIndex() const { return row_index_; }
  size_type FieldIndex() const { return field_index_; }

  //@{
  /** @name Field metadata */
  /// Field name as named in query
  std::string_view Name() const;
  FieldDescription Description() const;

  Oid GetTypeOid() const;
  //@}

  //@{
  /** @name Data access */
  bool IsNull() const;

  /// Read the field's buffer into user-provided variable.
  /// @throws FieldValueIsNull If the field is null and the C++ type is
  ///                           not nullable.
  template <typename T>
  size_type To(T&& val) const {
    using ValueType = typename std::decay<T>::type;
    auto fb = GetBuffer();
    return ReadNullable(fb, std::forward<T>(val),
                        io::traits::IsNullable<ValueType>{});
  }

  /// Read the field's buffer into user-provided variable.
  /// If the field is null, set the variable to the default value.
  template <typename T>
  void Coalesce(T& val, const T& default_val) const {
    if (!IsNull())
      To(val);
    else
      val = default_val;
  }

  /// Convert the field's buffer into a C++ type.
  /// @throws FieldValueIsNull If the field is null and the C++ type is
  ///                           not nullable.
  template <typename T>
  typename std::decay<T>::type As() const {
    T val{};
    To(val);
    return val;
  }

  /// Convert the field's buffer into a C++ type.
  /// If the field is null, return default value.
  template <typename T>
  typename std::decay<T>::type Coalesce(const T& default_val) const {
    if (IsNull()) return default_val;
    return As<T>();
  }
  //@}
  const io::TypeBufferCategory& GetTypeBufferCategories() const;

 private:
  io::FieldBuffer GetBuffer() const;

 protected:
  friend class Row;

  Field(detail::ResultWrapperPtr res, size_type row, size_type col)
      : res_{std::move(res)}, row_index_{row}, field_index_{col} {}

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
  size_type ReadNullable(const io::FieldBuffer& buffer, T&& val,
                         std::false_type) const {
    if (buffer.is_null) {
      throw FieldValueIsNull{field_index_, Name(), val};
    } else {
      Read(buffer, std::forward<T>(val));
    }
    return buffer.length;
  }

  //@{
  /** @name Iteration support */
  bool IsValid() const;
  int Compare(const Field& rhs) const;
  std::ptrdiff_t Distance(const Field& rhs) const;
  Field& Advance(std::ptrdiff_t);
  //@}

 private:
  template <typename T>
  void Read(const io::FieldBuffer& buffer, T&& val) const {
    using ValueType = typename std::decay<T>::type;
    io::traits::CheckParser<ValueType>();
    try {
      io::ReadBuffer(buffer, std::forward<T>(val), GetTypeBufferCategories());
    } catch (ResultSetError& ex) {
      ex.AddMsgSuffix(fmt::format(
          " (field #{} name `{}` C++ type `{}`. Postgres ResultSet error)",
          field_index_, Name(), compiler::GetTypeName<T>()));
      throw;
    }
  }

  detail::ResultWrapperPtr res_;
  size_type row_index_;
  size_type field_index_;
};

/// @brief Iterator over fields in a result set's row
class ConstFieldIterator
    : public detail::ConstDataIterator<ConstFieldIterator, Field,
                                       detail::IteratorDirection::kForward> {
  friend class Row;

  ConstFieldIterator(detail::ResultWrapperPtr res, size_type row, size_type col)
      : ConstDataIterator(std::move(res), row, col) {}
};

/// @brief Reverse iterator over fields in a result set's row
class ReverseConstFieldIterator
    : public detail::ConstDataIterator<ReverseConstFieldIterator, Field,
                                       detail::IteratorDirection::kReverse> {
  friend class Row;

  ReverseConstFieldIterator(detail::ResultWrapperPtr res, size_type row,
                            size_type col)
      : ConstDataIterator(std::move(res), row, col) {}
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
  using const_reverse_iterator = ReverseConstFieldIterator;

  using value_type = Field;
  using reference = Field;
  using pointer = const_iterator;
  //@}

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

  /// @brief Field access by index
  /// @throws FieldIndexOutOfBounds if index is out of bounds
  reference operator[](size_type index) const;
  /// @brief Field access field by name
  /// @throws FieldNameDoesntExist if the result set doesn't contain
  ///         such a field
  reference operator[](const std::string& name) const;
  //@}

  //@{
  /** @name Access to row's data */
  /// Read the contents of the row to a user's row type or read the first
  /// column into the value.
  ///
  /// If the user tries to read the first column into a variable, it must be the
  /// only column in the result set. If the result set contains more than one
  /// column, the function will throw NonSingleColumnResultSet. If the result
  /// set is OK to contain more than one columns, the first column value should
  /// be accessed via `row[0].To/As`.
  ///
  /// If the type is a 'row' type, the function will read the fields of the row
  /// into the type's data members.
  ///
  /// If the type can be treated as both a row type and a composite type (the
  /// type is mapped to a PostgreSQL type), the function will treat the type
  /// as a type for the first (and the only) column.
  ///
  /// To read the all fields of the row as a row type, the To(T&&, RowTag)
  /// should be used.
  template <typename T>
  void To(T&& val) const;

  /// Function to disambiguate reading the row to a user's row type (values
  /// of the row initialize user's type data members)
  template <typename T>
  void To(T&& val, RowTag) const;

  /// Function to disambiguate reading the first column to a user's composite
  /// type (PostgreSQL composite type in the row initializes user's type).
  /// The same as calling To(T&& val) for a T mapped to a PostgreSQL type.
  ///
  /// See @ref pg_composite_types
  template <typename T>
  void To(T&& val, FieldTag) const;

  /// Read fields into variables in order of their appearance in the row
  template <typename... T>
  void To(T&&... val) const;

  /// @brief Parse values from the row and return the result.
  ///
  /// If there are more than one type arguments to the function, it will
  /// return a tuple of those types.
  ///
  /// If there is a single type argument to the function, it will read the first
  /// and the only column of the row or the whole row to the row type (depending
  /// on C++ to PosgreSQL mapping presence) and return plain value of this type.
  ///
  /// @see To(T&&)
  template <typename T, typename... Y>
  auto As() const;

  /// @brief Returns T initialized with values of the row.
  /// @snippet storages/postgres/tests/typed_rows_pgtest.cpp RowTagSippet
  ///
  /// @see @ref pg_composite_types
  template <typename T>
  T As(RowTag) const {
    T val{};
    To(val, kRowTag);
    return val;
  }

  /// @brief Returns T initialized with a single column value of the row.
  /// @snippet storages/postgres/tests/composite_types_pgtest.cpp FieldTagSippet
  ///
  /// @see @ref pg_composite_types
  template <typename T>
  T As(FieldTag) const {
    T val{};
    To(val, kFieldTag);
    return val;
  }

  /// Read fields into variables in order of their names in the first argument
  template <typename... T>
  void To(const std::initializer_list<std::string>& names, T&&... val) const;
  template <typename... T>
  std::tuple<T...> As(const std::initializer_list<std::string>& names) const;

  /// Read fields into variables in order of their indexes in the first
  /// argument
  template <typename... T>
  void To(const std::initializer_list<size_type>& indexes, T&&... val) const;
  template <typename... T>
  std::tuple<T...> As(const std::initializer_list<size_type>& indexes) const;
  //@}

  size_type IndexOfName(const std::string&) const;

 protected:
  friend class ResultSet;

  Row(detail::ResultWrapperPtr res, size_type row)
      : res_{std::move(res)}, row_index_{row} {}

  //@{
  /** @name Iteration support */
  bool IsValid() const;
  int Compare(const Row& rhs) const;
  std::ptrdiff_t Distance(const Row& rhs) const;
  Row& Advance(std::ptrdiff_t);
  //@}
 private:
  detail::ResultWrapperPtr res_;
  size_type row_index_;
};

/// @name Iterator over rows in a result set
class ConstRowIterator
    : public detail::ConstDataIterator<ConstRowIterator, Row,
                                       detail::IteratorDirection::kForward> {
  friend class ResultSet;

  ConstRowIterator(detail::ResultWrapperPtr res, size_type row)
      : ConstDataIterator(std::move(res), row) {}
};

/// @name Reverse iterator over rows in a result set
class ReverseConstRowIterator
    : public detail::ConstDataIterator<ReverseConstRowIterator, Row,
                                       detail::IteratorDirection::kReverse> {
  friend class ResultSet;

  ReverseConstRowIterator(detail::ResultWrapperPtr res, size_type row)
      : ConstDataIterator(std::move(res), row) {}
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
  using const_reverse_iterator = ReverseConstRowIterator;

  using value_type = Row;
  using reference = value_type;
  using pointer = const_iterator;
  //@}

  explicit ResultSet(std::shared_ptr<detail::ResultWrapper> pimpl)
      : pimpl_{std::move(pimpl)} {}

  /// Number of rows in the result set
  size_type Size() const;
  bool IsEmpty() const { return Size() == 0; }

  size_type RowsAffected() const;
  std::string CommandStatus() const;

  //@{
  /** @name Row container interface */
  //@{
  /** @name Forward iteration */
  const_iterator cbegin() const&;
  const_iterator begin() const& { return cbegin(); }
  const_iterator cend() const&;
  const_iterator end() const& { return cend(); }

  // One should store ResultSet before using its accessors
  const_iterator cbegin() const&& = delete;
  const_iterator begin() const&& = delete;
  const_iterator cend() const&& = delete;
  const_iterator end() const&& = delete;
  //@}
  //@{
  /** @name Reverse iteration */
  const_reverse_iterator crbegin() const&;
  const_reverse_iterator rbegin() const& { return crbegin(); }
  const_reverse_iterator crend() const&;
  const_reverse_iterator rend() const& { return crend(); }
  // One should store ResultSet before using its accessors
  const_reverse_iterator crbegin() const&& = delete;
  const_reverse_iterator rbegin() const&& = delete;
  const_reverse_iterator crend() const&& = delete;
  const_reverse_iterator rend() const&& = delete;
  //@}

  reference Front() const&;
  reference Back() const&;
  // One should store ResultSet before using its accessors
  reference Front() const&& = delete;
  reference Back() const&& = delete;

  /// @brief Access a row by index
  /// @throws RowIndexOutOfBounds if index is out of bounds
  reference operator[](size_type index) const&;
  // One should store ResultSet before using its accessors
  reference operator[](size_type index) const&& = delete;
  //@}

  //@{
  /** @name ResultSet metadata access */
  // TODO ResultSet metadata access interface
  size_type FieldCount() const;
  RowDescription GetRowDescription() const& { return {pimpl_}; }
  // One should store ResultSet before using its accessors
  RowDescription GetRowDescription() const&& = delete;
  //@}

  //@{
  /** @name Typed results */
  /// @brief Get a wrapper for iterating over a set of typed results.
  /// For more information see @ref psql_typed_results
  template <typename T>
  auto AsSetOf() const;
  template <typename T>
  auto AsSetOf(RowTag) const;
  template <typename T>
  auto AsSetOf(FieldTag) const;

  /// @brief Extract data into a container.
  /// For more information see @ref psql_typed_results
  template <typename Container>
  Container AsContainer() const;
  template <typename Container>
  Container AsContainer(RowTag) const;

  /// @brief Extract first row into user type.
  /// A single row result set is expected, will throw an exception when result
  /// set size != 1
  template <typename T>
  auto AsSingleRow() const;
  template <typename T>
  auto AsSingleRow(RowTag) const;
  template <typename T>
  auto AsSingleRow(FieldTag) const;
  //@}
 private:
  friend class detail::ConnectionImpl;
  void FillBufferCategories(const UserTypes& types);
  void SetBufferCategoriesFrom(const ResultSet&);

  template <typename T, typename Tag>
  friend class TypedResultSet;
  friend class ConnectionImpl;

  std::shared_ptr<detail::ResultWrapper> pimpl_;
};

namespace detail {

//@{
/** @name Sequental field extraction */
template <typename IndexTuple, typename... T>
struct RowDataExtractorBase;

template <std::size_t... Indexes, typename... T>
struct RowDataExtractorBase<std::index_sequence<Indexes...>, T...> {
  static void ExtractValues(const Row& row, T&&... val) {
    static_assert(sizeof...(Indexes) == sizeof...(T));

    // We do it this way instead of row[Indexes...] to avoid Row::operator[]
    // overhead - it copies + destroys a shared_ptr to ResultSet
    auto it = row.begin();
    const auto perform = [&](auto&& arg) {
      it->To(std::forward<decltype(arg)>(arg));
      ++it;
    };
    (perform(std::forward<T>(val)), ...);
  }
  static void ExtractTuple(const Row& row, std::tuple<T...>& val) {
    static_assert(sizeof...(Indexes) == sizeof...(T));

    // We do it this way instead of row[Indexes...] to avoid Row::operator[]
    // overhead - it copies + destroys a shared_ptr to ResultSet
    auto it = row.begin();
    const auto perform = [&](auto& arg) {
      it->To(arg);
      ++it;
    };
    (perform(std::get<Indexes>(val)), ...);
  }
  static void ExtractTuple(const Row& row, std::tuple<T...>&& val) {
    static_assert(sizeof...(Indexes) == sizeof...(T));

    // We do it this way instead of row[Indexes...] to avoid Row::operator[]
    // overhead - it copies + destroys a shared_ptr to ResultSet
    auto it = row.begin();
    const auto perform = [&](auto& arg) {
      it->To(arg);
      ++it;
    };
    (perform(std::get<Indexes>(val)), ...);
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

template <typename T>
struct TupleDataExtractor;
template <typename... T>
struct TupleDataExtractor<std::tuple<T...>>
    : RowDataExtractorBase<std::index_sequence_for<T...>, T...> {};
//@}

}  // namespace detail

template <typename T>
void Row::To(T&& val) const {
  To(std::forward<T>(val), kFieldTag);
}

template <typename T>
void Row::To(T&& val, RowTag) const {
  // Convert the val into a writable tuple and extract the data
  using ValueType = std::decay_t<T>;
  static_assert(io::traits::kIsRowType<ValueType>,
                "This type cannot be used as a row type");
  using RowType = io::RowType<ValueType>;
  using TupleType = typename RowType::TupleType;
  constexpr auto tuple_size = RowType::size;
  if (tuple_size > Size()) {
    throw InvalidTupleSizeRequested(Size(), tuple_size);
  } else if (tuple_size < Size()) {
    LOG_LIMITED_WARNING()
        << "Row size is greater that the number of data members in "
           "C++ user datatype "
        << compiler::GetTypeName<T>();
  }

  detail::TupleDataExtractor<TupleType>::ExtractTuple(
      *this, RowType::GetTuple(std::forward<T>(val)));
}

template <typename T>
void Row::To(T&& val, FieldTag) const {
  using ValueType = std::decay_t<T>;
  // composite types can be parsed without an explicit mapping
  static_assert(io::traits::kIsMappedToPg<ValueType> ||
                    io::traits::kIsCompositeType<ValueType>,
                "This type is not mapped to a PostgreSQL type");
  // Read the first field into the type
  if (Size() < 1) {
    throw InvalidTupleSizeRequested{Size(), 1};
  }
  if (Size() > 1) {
    throw NonSingleColumnResultSet{Size(), compiler::GetTypeName<T>(), "As"};
  }
  (*this)[0].To(std::forward<T>(val));
}

template <typename... T>
void Row::To(T&&... val) const {
  if (sizeof...(T) > Size()) {
    throw InvalidTupleSizeRequested(Size(), sizeof...(T));
  }
  detail::RowDataExtractor<T...>::ExtractValues(*this, std::forward<T>(val)...);
}

template <typename T, typename... Y>
auto Row::As() const {
  if constexpr (sizeof...(Y) > 0) {
    std::tuple<T, Y...> res;
    To(res, kRowTag);
    return res;
  } else {
    return As<T>(kFieldTag);
  }
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
std::tuple<T...> Row::As(
    const std::initializer_list<std::string>& names) const {
  if (sizeof...(T) != names.size()) {
    throw FieldTupleMismatch(names.size(), sizeof...(T));
  }
  std::tuple<T...> res;
  detail::RowDataExtractor<T...>::ExtractTuple(*this, names, res);
  return res;
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
std::tuple<T...> Row::As(
    const std::initializer_list<size_type>& indexes) const {
  if (sizeof...(T) != indexes.size()) {
    throw FieldTupleMismatch(indexes.size(), sizeof...(T));
  }
  std::tuple<T...> res;
  detail::RowDataExtractor<T...>::ExtractTuple(*this, indexes, res);
  return res;
}

template <typename T>
auto ResultSet::AsSetOf() const {
  return AsSetOf<T>(kFieldTag);
}

template <typename T>
auto ResultSet::AsSetOf(RowTag) const {
  using ValueType = std::decay_t<T>;
  static_assert(io::traits::kIsRowType<ValueType>,
                "This type cannot be used as a row type");
  return TypedResultSet<T, RowTag>{*this};
}

template <typename T>
auto ResultSet::AsSetOf(FieldTag) const {
  using ValueType = std::decay_t<T>;
  // composite types can be parsed without an explicit mapping
  static_assert(io::traits::kIsMappedToPg<ValueType> ||
                    io::traits::kIsCompositeType<ValueType>,
                "This type is not mapped to a PostgreSQL type");
  if (FieldCount() > 1) {
    throw NonSingleColumnResultSet{FieldCount(), compiler::GetTypeName<T>(),
                                   "AsSetOf"};
  }
  return TypedResultSet<T, FieldTag>{*this};
}

template <typename Container>
Container ResultSet::AsContainer() const {
  using ValueType = typename Container::value_type;
  Container c;
  if constexpr (io::traits::kCanReserve<Container>) {
    c.reserve(Size());
  }
  auto res = AsSetOf<ValueType>();
  std::copy(res.begin(), res.end(), io::traits::Inserter(c));
  return c;
}

template <typename Container>
Container ResultSet::AsContainer(RowTag) const {
  using ValueType = typename Container::value_type;
  Container c;
  if constexpr (io::traits::kCanReserve<Container>) {
    c.reserve(Size());
  }
  auto res = AsSetOf<ValueType>(kRowTag);
  std::copy(res.begin(), res.end(), io::traits::Inserter(c));
  return c;
}

template <typename T>
auto ResultSet::AsSingleRow() const {
  return AsSingleRow<T>(kFieldTag);
}

template <typename T>
auto ResultSet::AsSingleRow(RowTag) const {
  if (Size() != 1) {
    throw NonSingleRowResultSet{Size()};
  }
  return Front().As<T>(kRowTag);
}

template <typename T>
auto ResultSet::AsSingleRow(FieldTag) const {
  if (Size() != 1) {
    throw NonSingleRowResultSet{Size()};
  }
  return Front().As<T>(kFieldTag);
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END

#include <userver/storages/postgres/typed_result_set.hpp>
