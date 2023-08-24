#pragma once

/// @file userver/storages/postgres/typed_result_set.hpp
/// @brief Typed PostgreSQL results

#include <type_traits>

#include <userver/storages/postgres/detail/typed_rows.hpp>
#include <userver/storages/postgres/result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

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
/// For more information on supported data types please see @ref pg_types
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
/// ⇦ @ref pg_types | @ref pg_errors ⇨
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
  using const_iterator =
      detail::ConstTypedRowIterator<T, ExtractionTag,
                                    detail::IteratorDirection::kForward>;
  using const_reverse_iterator =
      detail::ConstTypedRowIterator<T, ExtractionTag,
                                    detail::IteratorDirection::kReverse>;

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
  const_iterator cend() const& {
    return const_iterator{result_.pimpl_, Size()};
  }
  const_iterator end() const& { return cend(); }
  const_iterator cbegin() const&& { ReportMisuse(); }
  const_iterator begin() const&& { ReportMisuse(); }
  const_iterator cend() const&& { ReportMisuse(); }
  const_iterator end() const&& { ReportMisuse(); }
  //@}
  //@{
  /** @name Reverse iteration */
  const_reverse_iterator crbegin() const& {
    return const_reverse_iterator(result_.pimpl_, Size() - 1);
  }
  const_reverse_iterator rbegin() const& { return crbegin(); }
  const_reverse_iterator crend() const& {
    return const_reverse_iterator(result_.pimpl_, npos);
  }
  const_reverse_iterator rend() const& { return crend(); }
  const_reverse_iterator crbegin() const&& { ReportMisuse(); }
  const_reverse_iterator rbegin() const&& { ReportMisuse(); }
  const_reverse_iterator crend() const&& { ReportMisuse(); }
  const_reverse_iterator rend() const&& { ReportMisuse(); }
  //@}
  /// @brief Access a row by index
  /// @throws RowIndexOutOfBounds if index is out of bounds
  // NOLINTNEXTLINE(readability-const-return-type)
  reference operator[](size_type index) const& {
    return result_[index].template As<value_type>(kExtractTag);
  }
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
