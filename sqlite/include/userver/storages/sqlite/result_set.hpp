#pragma once

/// @file userver/storages/sqlite/result_set.hpp
/// @copybrief storages::sqlite::ResultSet

#include <memory>
#include <optional>
#include <vector>

#include <userver/storages/sqlite/exceptions.hpp>
#include <userver/storages/sqlite/execution_result.hpp>
#include <userver/storages/sqlite/impl/extractor.hpp>
#include <userver/storages/sqlite/row_types.hpp>
#include <userver/storages/sqlite/sqlite_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

/// @brief A proxy for statement execution result.
///
/// This type can't be constructed in user code and is always retrieved from
/// storages::sqlite::Client, storages::sqlite::Transaction or storages::sqlite::Savepoint methods.
class ResultSet {
public:
    explicit ResultSet(impl::ResultWrapperPtr pimpl);

    ResultSet(const ResultSet& other) = delete;
    ResultSet(ResultSet&& other) noexcept;
    ResultSet& operator=(ResultSet&&) noexcept;

    ~ResultSet();

    // clang-format off
  /// @brief Parse statement result set as std::vector<T>.
  /// T is expected to be an aggregate of supported types.
  ///
  /// Throw exceptions on columns count mismatch or types mismatch.
  ///
    // clang-format on
    template <typename T>
    std::vector<T> AsVector() &&;

    // clang-format off
  /// @brief Parse statement result set as std::vector<T>.
  /// Result set is expected to have a single column, `T` is expected to be one
  /// of supported types.
  ///
  /// Throw exceptions on columns count not being equal to 1 or type mismatch.
  ///
    // clang-format on
    template <typename T>
    std::vector<T> AsVector(FieldTag) &&;

    // clang-format off
  /// @brief Parse statement result as T.
  /// Result set is expected to have a single row, `T` is expected to be an
  /// aggregate of supported types.
  ///
  /// Throw exceptions on columns count mismatch or types mismatch.
  /// throws if result set is empty or contains more than one row.
  ///
    // clang-format on
    template <typename T>
    T AsSingleRow() &&;

    // clang-format off
  /// @brief Parse statement result as T.
  /// Result set is expected to have a single row and a single column,
  /// `T` is expected to be one of supported types.
  ///
  /// Throw exceptions on columns count not being equal to 1 or type mismatch.
  /// throws if result set is empty of contains more than one row.
  ///
    // clang-format on
    template <typename T>
    T AsSingleField() &&;

    // clang-format off
  /// @brief Parse statement result as std::optional<T>.
  /// Result set is expected to have not more than one row,
  /// `T` is expected to be an aggregate of supported types.
  ///
  /// Throw exceptions on columns count mismatch or types mismatch.
  /// throws if result set contains more than one row.
  ///
    // clang-format on
    template <typename T>
    std::optional<T> AsOptionalSingleRow() &&;

    // clang-format off
  /// @brief Parse statement result as T.
  /// Result set is expected to have not more than one row,
  /// `T` is expected to be one of supported types.
  ///
  /// Throw exceptions on columns count not being equal to 1 or type mismatch.
  /// throws if result set contains more than one row.
  ///
    // clang-format on
    template <typename T>
    std::optional<T> AsOptionalSingleField() &&;

    /// @brief Get statement execution metadata.
    ExecutionResult AsExecutionResult() &&;

private:
    template <typename T>
    friend class CursorResultSet;

    void FetchAllResult(impl::ExtractorBase& extractor);

    bool FetchResult(impl::ExtractorBase& extractor, size_t batch_size);

    impl::ResultWrapperPtr pimpl_;
};

template <typename T>
std::vector<T> ResultSet::AsVector() && {
    impl::TypedExtractor<T, RowTag> extractor{*pimpl_};
    FetchAllResult(extractor);
    return extractor.ExtractData();
}

template <typename T>
std::vector<T> ResultSet::AsVector(FieldTag) && {
    impl::TypedExtractor<T, FieldTag> extractor{*pimpl_};
    FetchAllResult(extractor);
    return extractor.ExtractData();
}

template <typename T>
T ResultSet::AsSingleRow() && {
    auto optional_data = std::move(*this).AsOptionalSingleRow<T>();
    if (!optional_data.has_value()) {
        throw SQLiteException{"Result set is empty"};
    }
    return std::move(*optional_data);
}

template <typename T>
T ResultSet::AsSingleField() && {
    auto optional_data = std::move(*this).AsOptionalSingleField<T>();
    if (!optional_data.has_value()) {
        throw SQLiteException{"Result set is empty"};
    }
    return std::move(*optional_data);
}

template <typename T>
std::optional<T> ResultSet::AsOptionalSingleRow() && {
    auto rows = std::move(*this).AsVector<T>();
    if (rows.empty()) {
        return std::nullopt;
    }
    if (rows.size() > 1) {
        // TODO: Maybe better logging warning
        throw SQLiteException("Result set contains more than one row");
    }
    return {{std::move(rows.front())}};
}

template <typename T>
std::optional<T> ResultSet::AsOptionalSingleField() && {
    auto rows = std::move(*this).AsVector<T>(kFieldTag);
    if (rows.empty()) {
        return std::nullopt;
    }
    if (rows.size() > 1) {
        // TODO: Maybe better logging warning
        throw SQLiteException("Result set contains more than one row");
    }
    return {{std::move(rows.front())}};
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
