#include <storages/mysql/impl/bindings/input_bindings.hpp>

#include <date.h>
#include <fmt/format.h>

#include <userver/utils/assert.hpp>

#include <storages/mysql/assert_when_implemented.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::bindings {

InputBindings::InputBindings(std::size_t size)
    : owned_binds_{size}, binds_ptr_{owned_binds_.data()} {
  // Not always necessary, but we can live with that
  intermediate_buffers_.resize(size);
}

void InputBindings::SetParamsCallback(ParamsCallback params_cb) {
  params_cb_ = params_cb;
}

InputBindings::ParamsCallback InputBindings::GetParamsCallback() const {
  return params_cb_;
}

void InputBindings::SetUserData(void* user_data) { user_data_ = user_data; }

void* InputBindings::GetUserData() const { return user_data_; }

std::size_t InputBindings::Size() const { return owned_binds_.size(); }

bool InputBindings::Empty() const { return Size() == 0; }

MYSQL_BIND* InputBindings::GetBindsArray() {
  UASSERT(!Empty());

  return binds_ptr_;
}

void InputBindings::WrapBinds(void* binds_array) {
  UASSERT(binds_array);
  binds_ptr_ = static_cast<MYSQL_BIND*>(binds_array);
}

void InputBindings::Bind(std::size_t pos, C<uint8_t>& val) {
  BindValue(pos, GetNativeType(val), val, 0, true);
}
void InputBindings::Bind(std::size_t pos, C<O<uint8_t>>& val) {
  BindOptional(pos, val);
}
void InputBindings::Bind(std::size_t pos, C<int8_t>& val) {
  BindValue(pos, GetNativeType(val), val, 0);
}
void InputBindings::Bind(std::size_t pos, C<O<int8_t>>& val) {
  BindOptional(pos, val);
}

void InputBindings::Bind(std::size_t pos, C<uint16_t>& val) {
  BindValue(pos, GetNativeType(val), val, 0, true);
}
void InputBindings::Bind(std::size_t pos, C<O<uint16_t>>& val) {
  BindOptional(pos, val);
}
void InputBindings::Bind(std::size_t pos, C<int16_t>& val) {
  BindValue(pos, GetNativeType(val), val, 0);
}
void InputBindings::Bind(std::size_t pos, C<O<int16_t>>& val) {
  BindOptional(pos, val);
}

void InputBindings::Bind(std::size_t pos, C<uint32_t>& val) {
  BindValue(pos, GetNativeType(val), val, 0, true);
}
void InputBindings::Bind(std::size_t pos, C<O<uint32_t>>& val) {
  BindOptional(pos, val);
}
void InputBindings::Bind(std::size_t pos, C<int32_t>& val) {
  BindValue(pos, GetNativeType(val), val, 0);
}
void InputBindings::Bind(std::size_t pos, C<O<int32_t>>& val) {
  BindOptional(pos, val);
}

void InputBindings::Bind(std::size_t pos, C<uint64_t>& val) {
  BindValue(pos, GetNativeType(val), val, 0, true);
}
void InputBindings::Bind(std::size_t pos, C<O<uint64_t>>& val) {
  BindOptional(pos, val);
}
void InputBindings::Bind(std::size_t pos, C<int64_t>& val) {
  BindValue(pos, GetNativeType(val), val, 0);
}
void InputBindings::Bind(std::size_t pos, C<O<int64_t>>& val) {
  BindOptional(pos, val);
}

void InputBindings::Bind(std::size_t pos, C<float>& val) {
  BindValue(pos, GetNativeType(val), val, 0);
}
void InputBindings::Bind(std::size_t pos, C<O<float>>& val) {
  BindOptional(pos, val);
}
void InputBindings::Bind(std::size_t pos, C<double>& val) {
  BindValue(pos, GetNativeType(val), val, 0);
}
void InputBindings::Bind(std::size_t pos, C<O<double>>& val) {
  BindOptional(pos, val);
}

void InputBindings::Bind(std::size_t pos, C<io::DecimalWrapper>& val) {
  BindDecimal(pos, val);
}
void InputBindings::Bind(std::size_t pos, C<O<io::DecimalWrapper>>& val) {
  BindOptional(pos, val);
}

void InputBindings::Bind(std::size_t pos, C<std::string>& val) {
  Bind(pos, std::string_view{val});
}
void InputBindings::Bind(std::size_t pos, C<O<std::string>>& val) {
  BindOptional(pos, val);
}
void InputBindings::Bind(std::size_t pos, C<std::string_view>& val) {
  BindStringView(pos, val);
}
void InputBindings::Bind(std::size_t pos, C<O<std::string_view>>& val) {
  BindOptional(pos, val);
}

void InputBindings::Bind(std::size_t pos, C<formats::json::Value>& val) {
  BindJson(pos, val);
}
void InputBindings::Bind(std::size_t pos, C<O<formats::json::Value>>& val) {
  BindOptional(pos, val);
}

void InputBindings::Bind(std::size_t pos,
                         C<std::chrono::system_clock::time_point>& val) {
  BindDate(pos, val);
}
void InputBindings::Bind(std::size_t pos,
                         C<O<std::chrono::system_clock::time_point>>& val) {
  BindOptional(pos, val);
}

void InputBindings::BindNull(std::size_t pos) {
  UASSERT(pos < Size());

  auto& bind = GetBind(pos);
  bind.buffer_type = MYSQL_TYPE_NULL;
}

void InputBindings::BindDate(std::size_t pos,
                             C<std::chrono::system_clock::time_point>& val) {
  UASSERT(pos < Size());

  // https://stackoverflow.com/a/15958113/10508079
  auto dp = date::floor<date::days>(val);
  auto ymd = date::year_month_day{date::floor<date::days>(val)};
  auto time = date::make_time(
      std::chrono::duration_cast<MariaDBTimepointResolution>(val - dp));

  auto& native_time = intermediate_buffers_[pos].time;
  native_time.year = ymd.year().operator int();
  native_time.month = ymd.month().operator unsigned int();
  native_time.day = ymd.day().operator unsigned int();
  native_time.hour = time.hours().count();
  native_time.minute = time.minutes().count();
  native_time.second = time.seconds().count();
  native_time.second_part = time.subseconds().count();

  native_time.time_type = MYSQL_TIMESTAMP_DATETIME;

  BindValue(pos, GetNativeType(val), native_time, sizeof(MYSQL_TIME));
}

void InputBindings::BindStringView(std::size_t pos, C<std::string_view>& val) {
  UASSERT(pos < Size());

  const void* buffer = val.data();

  auto& bind = GetBind(pos);
  bind.buffer_type = MYSQL_TYPE_STRING;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  bind.buffer = const_cast<void*>(buffer);
  bind.buffer_length = val.length();
}

void InputBindings::BindJson(std::size_t pos, C<formats::json::Value>& val) {
  UASSERT(pos < Size());

  auto& json_str = intermediate_buffers_[pos].string;
  json_str = ToString(val);

  auto& bind = GetBind(pos);
  // This should be MYSQL_TYPE_JSON ideally, but
  // https://bugs.mysql.com/bug.php?id=95984
  // MariaDB doesn't allow for json in params either
  // https://github.com/MariaDB/server/blob/10.11/sql/sql_prepare.cc#L933
  bind.buffer_type = MYSQL_TYPE_STRING;
  bind.buffer = json_str.data();
  bind.buffer_length = json_str.length();
}

void InputBindings::BindDecimal(std::size_t pos, C<io::DecimalWrapper>& val) {
  UASSERT(pos < Size());

  auto& decimal_str = intermediate_buffers_[pos].string;
  decimal_str = val.GetValue();

  auto& bind = GetBind(pos);
  bind.buffer_type = MYSQL_TYPE_DECIMAL;
  bind.buffer = decimal_str.data();
  bind.buffer_length = decimal_str.length();
}

void InputBindings::ValidateAgainstStatement(MYSQL_STMT& statement) {
  const auto params_count = mysql_stmt_param_count(&statement);
  UINVARIANT(params_count == Size(),
             fmt::format("Parameters count mismatch: expected {}, got {}",
                         params_count, Size()));
  if (params_count == 0) {
    return;
  }

  const auto res_deleter = [](MYSQL_RES* res) { mysql_free_result(res); };
  std::unique_ptr<MYSQL_RES, decltype(res_deleter)> params_metadata{
      mysql_stmt_param_metadata(&statement), res_deleter};

  // mysql_stmt_param_metadata always returns NULL at the time of writing,
  // because "server doesn't deliver any information yet".
  // This is sad, but oh well
  AssertWhenImplemented(params_metadata != nullptr,
                        "mysql_stmt_param_metadata");
}

MYSQL_BIND& InputBindings::GetBind(std::size_t pos) {
  UASSERT(pos < Size());
  return binds_ptr_[pos];
}

}  // namespace storages::mysql::impl::bindings

USERVER_NAMESPACE_END
