#include <userver/storages/odbc/command_result_set.hpp>

#include <userver/utils/assert.hpp>

#include <storages/odbc/impl/query_result.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

class CommandResultSet::Impl final {
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

CommandResultSet::CommandResultSet(impl::QueryResult&& query_result)
    : impl_{std::move(query_result)} {}

CommandResultSet::CommandResultSet(CommandResultSet&& other) noexcept = default;

CommandResultSet::~CommandResultSet() = default;

std::size_t CommandResultSet::RowsCount() const { return impl_->RowsCount(); }

std::size_t CommandResultSet::FieldsCount() const { return impl_->FieldsCount(); }

bool CommandResultSet::Empty() const { return RowsCount() == 0; }

const std::string& CommandResultSet::At(std::size_t row, std::size_t column) const {
    return impl_->At(row, column);
}

std::string& CommandResultSet::At(std::size_t row, std::size_t column) {
    return impl_->At(row, column);
}

}  // namespace storages::odbc

USERVER_NAMESPACE_END 