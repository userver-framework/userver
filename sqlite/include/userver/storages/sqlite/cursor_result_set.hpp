#pragma once

/// @file userver/storages/sqlite/cursor_result_set.hpp

#include <userver/storages/sqlite/result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

/// @brief A wrapper for read-only cursor.
///
/// You should always retrieve it from `storages::sqlite::Client` for correct
/// behavior.
template <typename T>
class CursorResultSet final {
public:
    explicit CursorResultSet(ResultSet&& result_set, size_t batch_size);
    ~CursorResultSet();

    CursorResultSet(const CursorResultSet& other) = delete;
    CursorResultSet(CursorResultSet&& other) noexcept;

    /// @brief Fetches all the rows from cursor and for each new row executes
    /// row_callback.
    ///
    /// Usable when the result set is expected to be big enough to put too
    /// much memory pressure if fetched as a whole.
    template <typename RowCallback>
    void ForEach(RowCallback&& row_callback) &&;

private:
    ResultSet result_set_;
    size_t batch_size_;
};

template <typename T>
CursorResultSet<T>::CursorResultSet(ResultSet&& result_set, size_t batch_size)
    : result_set_{std::move(result_set)}, batch_size_{batch_size} {}

template <typename T>
CursorResultSet<T>::CursorResultSet(CursorResultSet<T>&& other) noexcept = default;

template <typename T>
CursorResultSet<T>::~CursorResultSet() = default;

template <typename T>
template <typename RowCallback>
void CursorResultSet<T>::ForEach(RowCallback&& row_callback) && {
    using IntermediateStorage = std::vector<T>;

    bool keep_going = true;
    impl::TypedExtractor<T, RowTag> extractor{*result_set_.pimpl_};

    while (keep_going) {
        keep_going = result_set_.FetchResult(extractor, batch_size_);

        IntermediateStorage data{extractor.ExtractData()};
        for (auto&& row : data) {
            row_callback(std::move(row));
        }
    }
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
