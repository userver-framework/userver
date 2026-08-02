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

std::int64_t Field::GetSignedIntegerForAs() const { return res_->GetSignedIntegerStrict(row_index_, field_index_); }

std::uint64_t Field::GetUnsignedIntegerForAs() const {
    return res_->GetUnsignedIntegerStrict(row_index_, field_index_);
}

double Field::GetFloatingPointForAs() const { return res_->GetFloatingPointStrict(row_index_, field_index_); }

std::string Field::GetStringForAs() const { return res_->GetStringStrict(row_index_, field_index_); }

bool Field::GetBoolForAs() const { return res_->GetBoolStrict(row_index_, field_index_); }

Bytes Field::GetBytesForAs() const { return res_->GetBytesStrict(row_index_, field_index_); }

Date Field::GetDateForAs() const { return res_->GetDateStrict(row_index_, field_index_); }

Time Field::GetTimeForAs() const { return res_->GetTimeStrict(row_index_, field_index_); }

Timestamp Field::GetTimestampForAs() const { return res_->GetTimestampStrict(row_index_, field_index_); }

std::string Field::GetDecimalForAs(std::size_t precision, std::size_t scale) const {
    return res_->GetDecimalStrict(row_index_, field_index_, precision, scale);
}

}  // namespace storages::odbc

USERVER_NAMESPACE_END
