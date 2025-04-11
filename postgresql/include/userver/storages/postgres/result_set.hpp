#pragma once

/// @file userver/storages/postgres/result_set.hpp
/// @brief Result accessors

#include <limits>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

#include <fmt/format.h>

#include <userver/storages/postgres/detail/typed_rows.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/supported_types.hpp>
#include <userver/storages/postgres/row.hpp>

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
/// C++ variable of supported type. Please see
/// @ref scripts/docs/en/userver/pg_types.md for more information on supported
/// types.
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
/// information on data type requirements see @ref pg_user_row_types
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
/// ⇦ @ref pg_run_queries | @ref scripts/docs/en/userver/pg_types.md ⇨
/// @htmlonly </div> @endhtmlonly

template <typename T, typename ExtractionTag>
class TypedResultSet;

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

    explicit ResultSet(std::shared_ptr<detail::ResultWrapper> pimpl) : pimpl_{std::move(pimpl)} {}

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
    /// For more information see @ref pg_user_row_types
    template <typename T>
    auto AsSetOf() const;
    template <typename T>
    auto AsSetOf(RowTag) const;
    template <typename T>
    auto AsSetOf(FieldTag) const;

    /// @brief Extract data into a container.
    /// For more information see @ref pg_user_row_types
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

    /// @brief Extract first row into user type.
    /// @returns A single row result set if non empty result was returned, empty
    /// std::optional otherwise
    /// @throws exception when result set size > 1
    template <typename T>
    std::optional<T> AsOptionalSingleRow() const;
    template <typename T>
    std::optional<T> AsOptionalSingleRow(RowTag) const;
    template <typename T>
    std::optional<T> AsOptionalSingleRow(FieldTag) const;
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

template <typename T>
auto ResultSet::AsSetOf() const {
    return AsSetOf<T>(kFieldTag);
}

template <typename T>
auto ResultSet::AsSetOf(RowTag) const {
    detail::AssertSaneTypeToDeserialize<T>();
    using ValueType = std::decay_t<T>;
    io::traits::AssertIsValidRowType<ValueType>();
    return TypedResultSet<T, RowTag>{*this};
}

template <typename T>
auto ResultSet::AsSetOf(FieldTag) const {
    detail::AssertSaneTypeToDeserialize<T>();
    using ValueType = std::decay_t<T>;
    detail::AssertRowTypeIsMappedToPgOrIsCompositeType<ValueType>();
    if (FieldCount() > 1) {
        throw NonSingleColumnResultSet{FieldCount(), compiler::GetTypeName<T>(), "AsSetOf"};
    }
    return TypedResultSet<T, FieldTag>{*this};
}

template <typename Container>
Container ResultSet::AsContainer() const {
    detail::AssertSaneTypeToDeserialize<Container>();
    using ValueType = typename Container::value_type;
    Container c;
    if constexpr (io::traits::kCanReserve<Container>) {
        c.reserve(Size());
    }
    auto res = AsSetOf<ValueType>();

    auto inserter = io::traits::Inserter(c);
    auto row_it = res.begin();
    for (std::size_t i = 0; i < res.Size(); ++i, ++row_it, ++inserter) {
        *inserter = *row_it;
    }

    return c;
}

template <typename Container>
Container ResultSet::AsContainer(RowTag) const {
    detail::AssertSaneTypeToDeserialize<Container>();
    using ValueType = typename Container::value_type;
    Container c;
    if constexpr (io::traits::kCanReserve<Container>) {
        c.reserve(Size());
    }
    auto res = AsSetOf<ValueType>(kRowTag);

    auto inserter = io::traits::Inserter(c);
    auto row_it = res.begin();
    for (std::size_t i = 0; i < res.Size(); ++i, ++row_it, ++inserter) {
        *inserter = *row_it;
    }

    return c;
}

template <typename T>
auto ResultSet::AsSingleRow() const {
    return AsSingleRow<T>(kFieldTag);
}

template <typename T>
auto ResultSet::AsSingleRow(RowTag) const {
    detail::AssertSaneTypeToDeserialize<T>();
    if (Size() != 1) {
        throw NonSingleRowResultSet{Size()};
    }
    return Front().As<T>(kRowTag);
}

template <typename T>
auto ResultSet::AsSingleRow(FieldTag) const {
    detail::AssertSaneTypeToDeserialize<T>();
    if (Size() != 1) {
        throw NonSingleRowResultSet{Size()};
    }
    return Front().As<T>(kFieldTag);
}

template <typename T>
std::optional<T> ResultSet::AsOptionalSingleRow() const {
    return AsOptionalSingleRow<T>(kFieldTag);
}

template <typename T>
std::optional<T> ResultSet::AsOptionalSingleRow(RowTag) const {
    return IsEmpty() ? std::nullopt : std::optional<T>{AsSingleRow<T>(kRowTag)};
}

template <typename T>
std::optional<T> ResultSet::AsOptionalSingleRow(FieldTag) const {
    return IsEmpty() ? std::nullopt : std::optional<T>{AsSingleRow<T>(kFieldTag)};
}

/// @page pg_user_row_types uPg: Typed PostgreSQL results
///
/// The ResultSet provides access to a generic PostgreSQL result buffer wrapper
/// with access to individual column buffers and means to parse the buffers into
/// a certain type.
///
/// For a user that wishes to get the results in a form of a sequence or a
/// container of C++ tuples or structures, the driver provides a way to coerce
/// the generic result set into a typed result set or a container of tuples or
/// structures that fulfill certain conditions.
///
/// TypedResultSet provides container interface for typed result rows for
/// iteration or random access without converting all the result set at once.
/// The iterators in the TypedResultSet satisfy requirements for a constant
/// RandomAccessIterator with the exception of dereferencing iterators.
///
/// @warning The operator* of the iterators returns value (not a reference to
/// it) and the iterators don't have the operator->.
///
/// @par Data row extraction
///
/// The data rows can be obtained as:
///   - std::tuple;
///   - aggregate class as is;
///   - non-aggregate class with some augmentation.
///
/// Data members of the tuple or the classes must be supported by the driver.
/// For more information on supported data types please see
/// @ref scripts/docs/en/userver/pg_types.md.
///
/// @par std::tuple.
///
/// The first option is to convert ResultSet's row to std::tuples.
///
/// ```
/// using MyRowType = std::tuple<int, string>;
/// auto trx = ...;
/// auto generic_result = trx.Execute("select a, b from my_table");
/// auto iteration = generic_result.AsSetOf<MyRowType>();
/// for (auto row : iteration) {
///   static_assert(std::is_same_v<decltype(row), MyRowType>,
///       "Iterate over tuples");
///   auto [a, b] = row;
///   std::cout << "a = " << a << "; b = " << b << "\n";
/// }
///
/// auto data = geric_result.AsContainer<std::vector<MyRowType>>();
/// ```
///
/// @par Aggregate classes.
///
/// A data row can be coerced to an aggregate class.
///
/// An aggregate class (C++03 8.5.1 §1) is a class that with no base classes, no
/// protected or private non-static data members, no user-declared constructors
/// and no virtual functions.
///
/// ```
/// struct MyRowType {
///   int a;
///   std::string b;
/// };
/// auto generic_result = trx.Execute("select a, b from my_table");
/// auto iteration = generic_result.AsSetOf<MyRowType>();
/// for (auto row : iteration) {
///   static_assert(std::is_same_v<decltype(row), MyRowType>,
///       "Iterate over aggregate classes");
///   std::cout << "a = " << row.a << "; b = " << row.b << "\n";
/// }
///
/// auto data = geric_result.AsContainer<std::vector<MyRowType>>();
/// ```
///
/// @par Non-aggregate classes.
///
/// Classes that do not satisfy the aggregate class requirements can be used
/// to be created from data rows by providing additional `Introspect` non-static
/// member function. The function should return a tuple of references to
/// member data fields. The class must be default constructible.
///
/// ```
/// class MyRowType {
///  private:
///   int a_;
///   std::string b_;
///  public:
///   MyRowType() = default; // default ctor is required
///   explicit MyRowType(int x);
///
///   auto Introspect() {
///     return std::tie(a_, b_);
///   }
///   int GetA() const;
///   const std::string& GetB() const;
/// };
///
/// auto generic_result = trx.Execute("select a, b from my_table");
/// auto iteration = generic_result.AsSetOf<MyRowType>();
/// for (auto row : iteration) {
///   static_assert(std::is_same_v<decltype(row), MyRowType>,
///       "Iterate over non-aggregate classes");
///   std::cout << "a = " << row.GetA() << "; b = " << row.GetB() << "\n";
/// }
///
/// auto data = geric_result.AsContainer<std::vector<MyRowType>>();
/// ```
/// @par Single-column result set
///
/// A single-column result set can be used to extract directly to the column
/// type. User types mapped to PostgreSQL will work as well. If you need to
/// extract the whole row into such a structure, you will need to disambiguate
/// the call with the kRowTag.
///
/// @code
/// auto string_set = generic_result.AsSetOf<std::string>();
/// std::string s = string_set[0];
///
/// auto string_vec = generic_result.AsContainer<std::vector<std::string>>();
///
/// // Extract first column into the composite type
/// auto foo_set = generic_result.AsSetOf<FooBar>();
/// auto foo_vec = generic_result.AsContainer<std::vector<FooBar>>();
///
/// // Extract the whole row, disambiguation
/// auto foo_set = generic_result.AsSetOf<FooBar>(kRowTag);
///
/// @endcode
///
///
/// ----------
///
/// @htmlonly <div class="bottom-nav"> @endhtmlonly
/// ⇦ @ref scripts/docs/en/userver/pg_types.md | @ref pg_errors ⇨
/// @htmlonly </div> @endhtmlonly

template <typename T, typename ExtractionTag>
class TypedResultSet {
public:
    using size_type = ResultSet::size_type;
    using difference_type = ResultSet::difference_type;
    static constexpr size_type npos = ResultSet::npos;
    static constexpr ExtractionTag kExtractTag{};

    //@{
    /** @name Row container concept */
    using const_iterator = detail::ConstTypedRowIterator<T, ExtractionTag, detail::IteratorDirection::kForward>;
    using const_reverse_iterator = detail::ConstTypedRowIterator<T, ExtractionTag, detail::IteratorDirection::kReverse>;

    using value_type = T;
    using pointer = const_iterator;

// Forbidding assignments to operator[] result in debug, getting max
// performance in release.
#ifdef NDEBUG
    using reference = value_type;
#else
    using reference = std::add_const_t<value_type>;
#endif

    //@}
    explicit TypedResultSet(ResultSet result) : result_{std::move(result)} {}

    /// Number of rows in the result set
    size_type Size() const { return result_.Size(); }
    bool IsEmpty() const { return Size() == 0; }
    //@{
    /** @name Container interface */
    //@{
    /** @name Row container interface */
    //@{
    /** @name Forward iteration */
    const_iterator cbegin() const& { return const_iterator{result_.pimpl_, 0}; }
    const_iterator begin() const& { return cbegin(); }
    const_iterator cend() const& { return const_iterator{result_.pimpl_, Size()}; }
    const_iterator end() const& { return cend(); }
    const_iterator cbegin() const&& { ReportMisuse(); }
    const_iterator begin() const&& { ReportMisuse(); }
    const_iterator cend() const&& { ReportMisuse(); }
    const_iterator end() const&& { ReportMisuse(); }
    //@}
    //@{
    /** @name Reverse iteration */
    const_reverse_iterator crbegin() const& { return const_reverse_iterator(result_.pimpl_, Size() - 1); }
    const_reverse_iterator rbegin() const& { return crbegin(); }
    const_reverse_iterator crend() const& { return const_reverse_iterator(result_.pimpl_, npos); }
    const_reverse_iterator rend() const& { return crend(); }
    const_reverse_iterator crbegin() const&& { ReportMisuse(); }
    const_reverse_iterator rbegin() const&& { ReportMisuse(); }
    const_reverse_iterator crend() const&& { ReportMisuse(); }
    const_reverse_iterator rend() const&& { ReportMisuse(); }
    //@}
    /// @brief Access a row by index
    /// @throws RowIndexOutOfBounds if index is out of bounds
    // NOLINTNEXTLINE(readability-const-return-type)
    reference operator[](size_type index) const& { return result_[index].template As<value_type>(kExtractTag); }
    // NOLINTNEXTLINE(readability-const-return-type)
    reference operator[](size_type) const&& { ReportMisuse(); }
    //@}
private:
    [[noreturn]] static void ReportMisuse() {
        static_assert(!sizeof(T), "keep the TypedResultSet before using, please");
    }

    ResultSet result_;
};

}  // namespace storages::postgres

USERVER_NAMESPACE_END
