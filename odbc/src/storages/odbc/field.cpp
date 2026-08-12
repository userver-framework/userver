#include <userver/storages/odbc/field.hpp>

#include <storages/odbc/detail/result_wrapper.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

std::string Field::GetString() const { return res_->GetString(row_index_, field_index_); }
int64_t Field::GetInt64() const { return res_->GetInt64(row_index_, field_index_); }
int32_t Field::GetInt32() const { return res_->GetInt32(row_index_, field_index_); }
double Field::GetDouble() const { return res_->GetDouble(row_index_, field_index_); }
bool Field::GetBool() const { return res_->GetBool(row_index_, field_index_); }
bool Field::IsNull() const { return res_->IsFieldNull(row_index_, field_index_); }

}  // namespace storages::odbc

USERVER_NAMESPACE_END
