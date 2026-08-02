#pragma once

/// @file userver/storages/odbc/result_set.hpp
/// @brief @copybrief storages::odbc::ResultSet

#include <concepts>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <boost/pfr/core.hpp>
#include <boost/pfr/traits.hpp>

#include <userver/storages/odbc/exception.hpp>
#include <userver/storages/odbc/odbc_fwd.hpp>
#include <userver/storages/odbc/row.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

/// @cond
namespace impl {

struct OdbcResultMappingTag;

template <typename T, std::size_t... Index>
constexpr bool AreResultMembersMappable(std::index_sequence<Index...>) {
    return sizeof...(Index) != 0 &&
           ((!std::is_reference_v<boost::pfr::tuple_element_t<Index, T>> &&
             kIsFieldAsType<std::remove_cv_t<boost::pfr::tuple_element_t<Index, T>>>) &&
            ...);
}

template <typename T>
constexpr bool DetectResultAggregate() {
    using Value = std::remove_cv_t<T>;
    if constexpr (std::is_class_v<Value> && std::is_aggregate_v<Value> && std::is_standard_layout_v<Value> &&
                  !std::is_union_v<Value> && io::traits::kAggregateHasNoBaseClass<Value> &&
                  boost::pfr::is_implicitly_reflectable_v<Value, OdbcResultMappingTag> && !kIsFieldAsType<Value> &&
                  !io::traits::kHasMappingDeclaration<Value>)
    {
        return AreResultMembersMappable<Value>(std::make_index_sequence<boost::pfr::tuple_size_v<Value>>{});
    } else {
        return false;
    }
}

template <typename T>
inline constexpr bool kIsResultAggregate = DetectResultAggregate<T>();

template <typename T>
inline constexpr bool kIsResultValue = kIsFieldAsType<std::remove_cv_t<T>> || kIsResultAggregate<T>;

template <typename Container>
concept ResultContainer =
    std::default_initializable<Container> && !std::same_as<std::remove_cv_t<Container>, std::string> &&
    requires { typename Container::value_type; } && kIsResultValue<typename Container::value_type> &&
    requires(Container& container, typename Container::value_type value) {
        container.insert(container.end(), std::move(value));
    };

}  // namespace impl
/// @endcond

/// @brief Result set for ODBC query execution
class ResultSet final {
public:
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    static constexpr size_type npos = std::numeric_limits<size_type>::max();

    //@{
    /** @name Row container concept */

    using value_type = Row;
    using reference = value_type;
    //@}

    explicit ResultSet(std::shared_ptr<detail::ResultWrapper> pimpl)
        : pimpl_{std::move(pimpl)}
    {}

    /// @brief Get the number of columns in the result set
    size_type FieldCount() const;

    size_type Size() const;

    /// @brief Number of rows affected by a data-modifying statement.
    /// Returns zero when the driver reports an unknown count.
    size_type RowsAffected() const;

    /// @brief Get a result column name by zero-based index.
    std::string_view GetFieldName(size_type index) const;

    /// @brief Check if the result set is empty
    bool IsEmpty() const;

    reference operator[](size_type index) const&;

    /// Materializes every row into the container's value type. Scalar values
    /// require exactly one result column; aggregate values are initialized in
    /// declaration order and require an exact column count.
    template <typename Container>
    requires impl::ResultContainer<Container>
    Container AsContainer() const;

    /// Materializes the only result row, requiring exactly one row.
    template <typename T>
    requires impl::kIsResultValue<T>
    T AsSingleRow() const;

    /// Returns no value for zero rows, materializes one row, and rejects more
    /// than one row. For optional-valued T the outer optional represents row
    /// presence and the inner optional represents SQL NULL.
    template <typename T>
    requires impl::kIsResultValue<T>
    std::optional<T> AsOptionalSingleRow() const;

private:
    template <typename T, std::size_t... Index>
    T MapAggregate(size_type row_index, std::index_sequence<Index...>) const;

    template <typename T>
    T MapRow(size_type row_index) const;

    std::shared_ptr<detail::ResultWrapper> pimpl_;
};

template <typename T, std::size_t... Index>
T ResultSet::MapAggregate(size_type row_index, std::index_sequence<Index...>) const {
    return T{
        operator[](row_index)[Index]
            .template As<std::remove_cvref_t<decltype(boost::pfr::get<Index>(std::declval<T&>()))>>()...
    };
}

template <typename T>
T ResultSet::MapRow(size_type row_index) const {
    using Value = std::remove_cv_t<T>;
    static_assert(impl::kIsResultValue<Value>, "Unsupported ODBC typed result value");

    if constexpr (impl::kIsFieldAsType<Value>) {
        if (FieldCount() != 1) {
            throw ResultSetError("ODBC scalar result mapping requires exactly one column");
        }
        return operator[](row_index)[0].template As<Value>();
    } else {
        constexpr auto kFieldCount = boost::pfr::tuple_size_v<Value>;
        if (FieldCount() != kFieldCount) {
            throw ResultSetError("ODBC aggregate result mapping requires exactly one column per aggregate member");
        }
        return MapAggregate<Value>(row_index, std::make_index_sequence<kFieldCount>{});
    }
}

template <typename Container>
requires impl::ResultContainer<Container>
Container ResultSet::AsContainer() const {
    using Value = typename Container::value_type;
    static_assert(impl::kIsResultValue<Value>, "Unsupported ODBC typed result container value");

    Container result;
    if constexpr (requires { result.reserve(Size()); }) {
        result.reserve(Size());
    }
    auto output = std::inserter(result, result.end());
    for (size_type index = 0; index < Size(); ++index) {
        *output++ = MapRow<Value>(index);
    }
    return result;
}

template <typename T>
requires impl::kIsResultValue<T>
T ResultSet::AsSingleRow() const {
    if (Size() != 1) {
        throw ResultSetError("ODBC single-row result mapping requires exactly one row");
    }
    return MapRow<T>(0);
}

template <typename T>
requires impl::kIsResultValue<T>
std::optional<T> ResultSet::AsOptionalSingleRow() const {
    if (Size() > 1) {
        throw ResultSetError("ODBC optional single-row result mapping accepts at most one row");
    }
    if (IsEmpty()) {
        return std::nullopt;
    }
    return std::optional<T>{MapRow<T>(0)};
}

}  // namespace storages::odbc

USERVER_NAMESPACE_END
