#include <userver/storages/odbc/statement_result_set.hpp>

#include <userver/utils/assert.hpp>

#include <storages/odbc/impl/query_result.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

class StatementResultSet::Impl final {
public:
    Impl(impl::QueryResult&& result) : result_{std::move(result)} {}
    ~Impl() = default;

    Impl(const Impl& other) = delete;
    Impl(Impl&& other) noexcept = default;

    std::size_t RowsCount() const { return result_.RowsCount(); }

    std::size_t FieldsCount() const {
        if (RowsCount() == 0) {
            return 0;
        }

        return result_.begin()->FieldsCount();
    }

    const std::string& At(std::size_t row, std::size_t column) const {
        UASSERT(row < RowsCount() && column < FieldsCount());
        return result_.GetRow(row).GetField(column);
    }

    std::string& At(std::size_t row, std::size_t column) {
        UASSERT(row < RowsCount() && column < FieldsCount());
        return result_.GetRow(row).GetField(column);
    }

private:
    impl::QueryResult result_;
};

StatementResultSet::StatementResultSet(impl::QueryResult&& query_result)
    : impl_{std::move(query_result)} {}

StatementResultSet::StatementResultSet(StatementResultSet&& other) noexcept = default;

StatementResultSet::~StatementResultSet() = default;

std::size_t StatementResultSet::RowsCount() const { return impl_->RowsCount(); }

std::size_t StatementResultSet::FieldsCount() const { return impl_->FieldsCount(); }

bool StatementResultSet::Empty() const { return RowsCount() == 0; }

const std::string& StatementResultSet::At(std::size_t row, std::size_t column) const {
    return impl_->At(row, column);
}

std::string& StatementResultSet::At(std::size_t row, std::size_t column) {
    return impl_->At(row, column);
}

}  // namespace storages::odbc

USERVER_NAMESPACE_END 