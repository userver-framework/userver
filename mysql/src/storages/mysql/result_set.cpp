#include <userver/storages/mysql/result_set.hpp>

#include <userver/utils/assert.hpp>

#include <storages/mysql/impl/mysql_result.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

class ResultSet::Impl final {
 public:
  Impl(impl::MySQLResult&& result) : result_{std::move(result)} {}
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

  std::string& GetAt(std::size_t row, std::size_t column) {
    UASSERT(row < RowsCount() && column < FieldsCount());

    return result_.GetRow(row).GetField(column);
  }

 private:
  impl::MySQLResult result_;
};

ResultSet::ResultSet(impl::MySQLResult&& mysql_result)
    : impl_{std::move(mysql_result)} {}

ResultSet::ResultSet(ResultSet&& other) noexcept = default;

ResultSet::~ResultSet() = default;

std::size_t ResultSet::RowsCount() const { return impl_->RowsCount(); }

std::size_t ResultSet::FieldsCount() const { return impl_->FieldsCount(); }

bool ResultSet::Empty() const { return RowsCount() == 0; }

std::string& ResultSet::GetAt(std::size_t row, std::size_t column) {
  return impl_->GetAt(row, column);
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
