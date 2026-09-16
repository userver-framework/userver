#pragma once

/// @file userver/storages/odbc/cursor.hpp
/// @brief @copybrief storages::odbc::Cursor

#include <cstddef>
#include <memory>

#include <userver/storages/odbc/result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

namespace detail {
class CursorImpl;
class ClusterImpl;
}  // namespace detail

/// @brief A move-only incremental ODBC result cursor.
///
/// Each Fetch materializes at most the requested number of rows into an owning
/// ResultSet. The cursor pins its connection until EOF, destruction, or an
/// error. This bounds memory materialized by userver, but an ODBC driver may
/// still buffer rows internally and server-side streaming is not guaranteed.
/// Fetch calls on a cursor must be sequential.
class Cursor final {
public:
    Cursor(const Cursor&) = delete;
    Cursor& operator=(const Cursor&) = delete;

    Cursor(Cursor&&) noexcept;
    Cursor& operator=(Cursor&&) noexcept;
    ~Cursor() noexcept;

    /// @brief Fetch up to @p rows and return an owning result chunk.
    /// @throws LogicError if rows is zero or the cursor is already terminal.
    ResultSet Fetch(std::size_t rows);

    /// @brief Whether EOF, an error, invalidation, or a moved-from state has
    /// made this cursor terminal.
    bool Done() const noexcept;

    /// @brief Number of rows returned by successful Fetch calls.
    std::size_t FetchedSoFar() const noexcept;

    explicit operator bool() const noexcept { return !Done(); }

private:
    friend class detail::ClusterImpl;
    friend class Transaction;

    explicit Cursor(std::shared_ptr<detail::CursorImpl> impl);

    std::shared_ptr<detail::CursorImpl> impl_;
};

}  // namespace storages::odbc

USERVER_NAMESPACE_END
