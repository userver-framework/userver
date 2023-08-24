#include <storages/mysql/impl/bindings/input_bindings.hpp>

#include <fmt/format.h>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::bindings {

InputBindings::InputBindings(std::size_t size)
    : owned_binds_{size}, binds_ptr_{owned_binds_.data()} {
  // Not always necessary, but we can live with that
  intermediate_buffers_.resize(size);
}

InputBindings::InputBindings(InputBindings&& other) noexcept
    : owned_binds_{std::move(other.owned_binds_)},
      intermediate_buffers_{std::move(other.intermediate_buffers_)},
      params_cb_{other.params_cb_},
      user_data_{other.user_data_},
      // Technically this is incorrect, since bind_ptr_ could've been updated to
      // point into libmariadb, but move neven happens after that
      binds_ptr_{owned_binds_.data()} {}

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
void InputBindings::Bind(std::size_t pos, C<int8_t>& val) {
  BindValue(pos, GetNativeType(val), val, 0);
}

void InputBindings::Bind(std::size_t pos, C<uint16_t>& val) {
  BindValue(pos, GetNativeType(val), val, 0, true);
}
void InputBindings::Bind(std::size_t pos, C<int16_t>& val) {
  BindValue(pos, GetNativeType(val), val, 0);
}

void InputBindings::Bind(std::size_t pos, C<uint32_t>& val) {
  BindValue(pos, GetNativeType(val), val, 0, true);
}
void InputBindings::Bind(std::size_t pos, C<int32_t>& val) {
  BindValue(pos, GetNativeType(val), val, 0);
}

void InputBindings::Bind(std::size_t pos, C<uint64_t>& val) {
  BindValue(pos, GetNativeType(val), val, 0, true);
}
void InputBindings::Bind(std::size_t pos, C<int64_t>& val) {
  BindValue(pos, GetNativeType(val), val, 0);
}

void InputBindings::Bind(std::size_t pos, C<float>& val) {
  BindValue(pos, GetNativeType(val), val, 0);
}
void InputBindings::Bind(std::size_t pos, C<double>& val) {
  BindValue(pos, GetNativeType(val), val, 0);
}

void InputBindings::Bind(std::size_t pos, C<io::DecimalWrapper>& val) {
  BindDecimal(pos, val);
}

void InputBindings::Bind(std::size_t pos, C<std::string>& val) {
  Bind(pos, std::string_view{val});
}
void InputBindings::Bind(std::size_t pos, C<std::string_view>& val) {
  BindStringView(pos, val);
}

void InputBindings::Bind(std::size_t pos, C<formats::json::Value>& val) {
  BindJson(pos, val);
}

void InputBindings::Bind(std::size_t pos,
                         C<std::chrono::system_clock::time_point>& val) {
  BindTimePoint(pos, val);
}

void InputBindings::Bind(std::size_t pos, C<Date>& val) { BindDate(pos, val); }
void InputBindings::Bind(std::size_t pos, C<DateTime>& val) {
  BindDateTime(pos, val);
}

void InputBindings::BindNull(std::size_t pos) {
  UASSERT(pos < Size());

  auto& bind = GetBind(pos);
  bind.buffer_type = MYSQL_TYPE_NULL;
}

void InputBindings::BindTimePoint(
    std::size_t pos, C<std::chrono::system_clock::time_point>& val) {
  UASSERT(pos < Size());

  auto& native_time = intermediate_buffers_[pos].time;
  native_time = ToNativeTime(val);
  native_time.time_type = MYSQL_TIMESTAMP_DATETIME;

  BindValue(pos, GetNativeType(val), native_time, sizeof(MYSQL_TIME));
}

void InputBindings::BindDate(std::size_t pos, C<Date>& val) {
  UASSERT(pos < Size());

  auto& native_time = intermediate_buffers_[pos].time;
  native_time.year = val.GetYear();
  native_time.month = val.GetMonth();
  native_time.day = val.GetDay();

  native_time.time_type = MYSQL_TIMESTAMP_DATE;

  BindValue(pos, GetNativeType(val), native_time, sizeof(MYSQL_TIME));
}

void InputBindings::BindDateTime(std::size_t pos, C<DateTime>& val) {
  UASSERT(pos < Size());

  auto& native_time = intermediate_buffers_[pos].time;
  native_time.year = val.GetDate().GetYear();
  native_time.month = val.GetDate().GetMonth();
  native_time.day = val.GetDate().GetDay();
  native_time.hour = val.GetHour();
  native_time.minute = val.GetMinute();
  native_time.second = val.GetSecond();
  native_time.second_part = val.GetMicrosecond();

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
  const std::unique_ptr<MYSQL_RES, decltype(res_deleter)> params_metadata{
      mysql_stmt_param_metadata(&statement), res_deleter};

  // mysql_stmt_param_metadata always returns NULL at the time of writing,
  // because "server doesn't deliver any information yet".
  // This is sad, but oh well
  if (params_metadata != nullptr) {
    LOG_LIMITED_WARNING()
        << "mysql_stmt_param_metadata got implemented serverside, please file "
           "an issue about it, so maintainers could improve the driver.";
  }
}

MYSQL_BIND& InputBindings::GetBind(std::size_t pos) {
  UASSERT(pos < Size());
  return binds_ptr_[pos];
}

}  // namespace storages::mysql::impl::bindings

USERVER_NAMESPACE_END
