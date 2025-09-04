#pragma once

/// @file
/// @brief Result row accessors

#include <initializer_list>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include <userver/storages/postgres/field.hpp>
#include <userver/storages/postgres/io/supported_types.hpp>
#include <userver/utils/zstring_view.hpp>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

class ResultSet;
template <typename T, typename ExtractionTag>
class TypedResultSet;

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
    reference operator[](USERVER_NAMESPACE::utils::zstring_view name) const;
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
    template <typename T>
    T As(RowTag) const {
        T val{};
        To(val, kRowTag);
        return val;
    }

    /// @brief Returns T initialized with a single column value of the row.
    /// @snippet storages/postgres/tests/composite_types_pgtest.cpp FieldTagSippet
    template <typename T>
    T As(FieldTag) const {
        T val{};
        To(val, kFieldTag);
        return val;
    }

    /// Read fields into variables in order of their names in the first argument
    template <typename... T>
    void To(const std::initializer_list<USERVER_NAMESPACE::utils::zstring_view>& names, T&&... val) const;
    template <typename... T>
    std::tuple<T...> As(const std::initializer_list<USERVER_NAMESPACE::utils::zstring_view>& names) const;

    /// Read fields into variables in order of their indexes in the first
    /// argument
    template <typename... T>
    void To(const std::initializer_list<size_type>& indexes, T&&... val) const;
    template <typename... T>
    std::tuple<T...> As(const std::initializer_list<size_type>& indexes) const;
    //@}

    size_type IndexOfName(USERVER_NAMESPACE::utils::zstring_view) const;

    FieldView GetFieldView(size_type index) const;

protected:
    friend class ResultSet;

    template <typename T, typename Tag>
    friend class TypedResultSet;

    Row() = default;

    Row(detail::ResultWrapperPtr res, size_type row) : res_{std::move(res)}, row_index_{row} {}

    //@{
    /** @name Iteration support */
    bool IsValid() const;
    int Compare(const Row& rhs) const;
    std::ptrdiff_t Distance(const Row& rhs) const;
    Row& Advance(std::ptrdiff_t);
    //@}
private:
    detail::ResultWrapperPtr res_;
    size_type row_index_{0};
};

/// @name Iterator over rows in a result set
class ConstRowIterator : public detail::ConstDataIterator<ConstRowIterator, Row, detail::IteratorDirection::kForward> {
public:
    ConstRowIterator() = default;

private:
    friend class ResultSet;

    ConstRowIterator(detail::ResultWrapperPtr res, size_type row) : ConstDataIterator(std::move(res), row) {}
};

/// @name Reverse iterator over rows in a result set
class ReverseConstRowIterator
    : public detail::ConstDataIterator<ReverseConstRowIterator, Row, detail::IteratorDirection::kReverse> {
public:
    ReverseConstRowIterator() = default;

private:
    friend class ResultSet;

    ReverseConstRowIterator(detail::ResultWrapperPtr res, size_type row) : ConstDataIterator(std::move(res), row) {}
};

namespace detail {

template <typename T>
struct IsOptionalFromOptional : std::false_type {};

template <typename T>
struct IsOptionalFromOptional<std::optional<std::optional<T>>> : std::true_type {};

template <typename T>
struct IsOneVariant : std::false_type {};

template <typename T>
struct IsOneVariant<std::variant<T>> : std::true_type {};

template <typename... Args>
constexpr void AssertSaneTypeToDeserialize() {
    static_assert(
        !(IsOptionalFromOptional<std::remove_const_t<std::remove_reference_t<Args>>>::value || ...),
        "Attempt to get an optional<optional<T>> was detected. Such "
        "optional-from-optional types are very error prone, obfuscate code and "
        "are ambiguous to deserialize. Change the type to just optional<T>"
    );
    static_assert(
        !(IsOneVariant<std::remove_const_t<std::remove_reference_t<Args>>>::value || ...),
        "Attempt to get an variant<T> was detected. Such variant from one type "
        "obfuscates code. Change the type to just T"
    );
}

//@{
/** @name Sequental field extraction */
template <typename IndexTuple, typename... T>
struct RowDataExtractorBase;

template <std::size_t... Indexes, typename... T>
struct RowDataExtractorBase<std::index_sequence<Indexes...>, T...> {
    static void ExtractValues(const Row& row, T&&... val) {
        static_assert(sizeof...(Indexes) == sizeof...(T));

        std::size_t field_index = 0;
        const auto perform = [&](auto&& arg) { row.GetFieldView(field_index++).To(std::forward<decltype(arg)>(arg)); };
        (perform(std::forward<T>(val)), ...);
    }
    static void ExtractTuple(const Row& row, std::tuple<T...>& val) {
        static_assert(sizeof...(Indexes) == sizeof...(T));

        std::size_t field_index = 0;
        const auto perform = [&](auto& arg) { row.GetFieldView(field_index++).To(arg); };
        (perform(std::get<Indexes>(val)), ...);
    }
    static void ExtractTuple(const Row& row, std::tuple<T...>&& val) {
        static_assert(sizeof...(Indexes) == sizeof...(T));

        std::size_t field_index = 0;
        const auto perform = [&](auto& arg) { row.GetFieldView(field_index++).To(arg); };
        (perform(std::get<Indexes>(val)), ...);
    }

    static void ExtractValues(
        const Row& row,
        const std::initializer_list<USERVER_NAMESPACE::utils::zstring_view>& names,
        T&&... val
    ) {
        (row[*(names.begin() + Indexes)].To(std::forward<T>(val)), ...);
    }
    static void ExtractTuple(
        const Row& row,
        const std::initializer_list<USERVER_NAMESPACE::utils::zstring_view>& names,
        std::tuple<T...>& val
    ) {
        std::tuple<T...> tmp{row[*(names.begin() + Indexes)].template As<T>()...};
        tmp.swap(val);
    }

    static void ExtractValues(const Row& row, const std::initializer_list<std::size_t>& indexes, T&&... val) {
        (row[*(indexes.begin() + Indexes)].To(std::forward<T>(val)), ...);
    }
    static void ExtractTuple(const Row& row, const std::initializer_list<std::size_t>& indexes, std::tuple<T...>& val) {
        std::tuple<T...> tmp{row[*(indexes.begin() + Indexes)].template As<T>()...};
        tmp.swap(val);
    }
};

template <typename... T>
struct RowDataExtractor : RowDataExtractorBase<std::index_sequence_for<T...>, T...> {};

template <typename T>
struct TupleDataExtractor;
template <typename... T>
struct TupleDataExtractor<std::tuple<T...>> : RowDataExtractorBase<std::index_sequence_for<T...>, T...> {};
//@}

template <typename RowType>
constexpr void AssertRowTypeIsMappedToPgOrIsCompositeType() {
    // composite types can be parsed without an explicit mapping
    static_assert(
        io::traits::kIsMappedToPg<RowType> || io::traits::kIsCompositeType<RowType>,
        "Row type must be mapped to pg type(CppToUserPg) or one of the "
        "following: "
        "1. primitive type. "
        "2. std::tuple. "
        "3. Aggregation type. See std::aggregation. "
        "4. Has a Introspect method that makes the std::tuple from your "
        "class/struct. "
        "For more info see `uPg: Typed PostgreSQL results` chapter in docs."
    );
}

}  // namespace detail

template <typename T>
void Row::To(T&& val) const {
    To(std::forward<T>(val), kFieldTag);
}

template <typename T>
void Row::To(T&& val, RowTag) const {
    detail::AssertSaneTypeToDeserialize<T>();
    // Convert the val into a writable tuple and extract the data
    using ValueType = std::decay_t<T>;
    io::traits::AssertIsValidRowType<ValueType>();
    using RowType = io::RowType<ValueType>;
    using TupleType = typename RowType::TupleType;
    constexpr auto tuple_size = RowType::size;
    if (tuple_size > Size()) {
        throw InvalidTupleSizeRequested(Size(), tuple_size);
    } else if (tuple_size < Size()) {
        LOG_LIMITED_WARNING() << "Row size is greater that the number of data members in "
                                 "C++ user datatype "
                              << compiler::GetTypeName<T>();
    }

    detail::TupleDataExtractor<TupleType>::ExtractTuple(*this, RowType::GetTuple(std::forward<T>(val)));
}

template <typename T>
void Row::To(T&& val, FieldTag) const {
    detail::AssertSaneTypeToDeserialize<T>();
    using ValueType = std::decay_t<T>;
    detail::AssertRowTypeIsMappedToPgOrIsCompositeType<ValueType>();
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
    detail::AssertSaneTypeToDeserialize<T...>();
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
void Row::To(const std::initializer_list<USERVER_NAMESPACE::utils::zstring_view>& names, T&&... val) const {
    detail::AssertSaneTypeToDeserialize<T...>();
    if (sizeof...(T) != names.size()) {
        throw FieldTupleMismatch(names.size(), sizeof...(T));
    }
    detail::RowDataExtractor<T...>::ExtractValues(*this, names, std::forward<T>(val)...);
}

template <typename... T>
std::tuple<T...> Row::As(const std::initializer_list<USERVER_NAMESPACE::utils::zstring_view>& names) const {
    if (sizeof...(T) != names.size()) {
        throw FieldTupleMismatch(names.size(), sizeof...(T));
    }
    std::tuple<T...> res;
    detail::RowDataExtractor<T...>::ExtractTuple(*this, names, res);
    return res;
}

template <typename... T>
void Row::To(const std::initializer_list<size_type>& indexes, T&&... val) const {
    detail::AssertSaneTypeToDeserialize<T...>();
    if (sizeof...(T) != indexes.size()) {
        throw FieldTupleMismatch(indexes.size(), sizeof...(T));
    }
    detail::RowDataExtractor<T...>::ExtractValues(*this, indexes, std::forward<T>(val)...);
}

template <typename... T>
std::tuple<T...> Row::As(const std::initializer_list<size_type>& indexes) const {
    if (sizeof...(T) != indexes.size()) {
        throw FieldTupleMismatch(indexes.size(), sizeof...(T));
    }
    std::tuple<T...> res;
    detail::RowDataExtractor<T...>::ExtractTuple(*this, indexes, res);
    return res;
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
