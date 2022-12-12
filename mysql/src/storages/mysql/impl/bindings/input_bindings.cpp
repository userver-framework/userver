#include <storages/mysql/impl/bindings/input_bindings.hpp>

#include <date.h>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::bindings {

InputBindings::InputBindings(std::size_t size) {
  binds_.resize(size);

  // Not always necessary, but we can live with that
  dates_.resize(size);
}

void InputBindings::SetParamsCallback(ParamsCallback params_cb) {
  params_cb_ = params_cb;
}

InputBindings::ParamsCallback InputBindings::GetParamsCallback() const {
  return params_cb_;
}

void InputBindings::SetUserData(void* user_data) { user_data_ = user_data; }

void* InputBindings::GetUserData() const { return user_data_; }

std::size_t InputBindings::Size() const { return binds_.size(); }

bool InputBindings::Empty() const { return Size() == 0; }

MYSQL_BIND* InputBindings::GetBindsArray() {
  UASSERT(!Empty());

  return binds_.data();
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

  auto& bind = binds_[pos];
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

  auto& native_time = dates_[pos];
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

void InputBindings::BindStringView(std::size_t pos,
                                   const std::string_view& val) {
  UASSERT(pos < Size());

  const void* buffer = val.data();

  auto& bind = binds_[pos];
  bind.buffer_type = MYSQL_TYPE_STRING;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  bind.buffer = const_cast<void*>(buffer);
  bind.buffer_length = val.length();
}

}  // namespace storages::mysql::impl::bindings

USERVER_NAMESPACE_END
