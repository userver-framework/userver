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

    explicit ResultSet(std::shared_ptr<detail::ResultWrapper> pimpl)
        : pimpl_{std::move(pimpl)}
    {}

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
    /// For more information see @ref scripts/docs/en/userver/pg/user_row_types.md
    template <typename T>
    auto AsSetOf() const;
    template <typename T>
    auto AsSetOf(RowTag) const;
    template <typename T>
    auto AsSetOf(FieldTag) const;

    /// @brief Extract data into a container.
    /// For more information see @ref scripts/docs/en/userver/pg/user_row_types.md
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
    using ValueType = std::remove_cvref_t<T>;
    io::traits::AssertIsValidRowType<ValueType>();
    return TypedResultSet<T, RowTag>{*this};
}

template <typename T>
auto ResultSet::AsSetOf(FieldTag) const {
    detail::AssertSaneTypeToDeserialize<T>();
    using ValueType = std::remove_cvref_t<T>;
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
    if constexpr (io::traits::CanReserve<Container>) {
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
    if constexpr (io::traits::CanReserve<Container>) {
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
    explicit TypedResultSet(ResultSet result)
        : result_{std::move(result)}
    {}

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
