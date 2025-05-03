#pragma once

#include <string>

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

namespace impl {
class QueryResult;
}

class CommandResultSet final {
public:
    explicit CommandResultSet(impl::QueryResult&& query_result);
    ~CommandResultSet();

    CommandResultSet(const CommandResultSet& other) = delete;
    CommandResultSet(CommandResultSet&& other) noexcept;

    std::size_t RowsCount() const;
    std::size_t FieldsCount() const;
    bool Empty() const;

    const std::string& At(std::size_t row, std::size_t column) const;
    std::string& At(std::size_t row, std::size_t column);

private:
    class Impl;
    utils::FastPimpl<Impl, 24, 8> impl_;
};

}  // namespace storages::odbc

USERVER_NAMESPACE_END