#include <storages/mysql/impl/bindings/output_bindings.hpp>

#include <date.h>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::bindings {

OutputBindings::OutputBindings(std::size_t size) {
  binds_.resize(size);

  // Not always necessary, but we can live with that
  callbacks_.resize(size);
  dates_.resize(size);
}

void OutputBindings::BeforeFetch(std::size_t pos) {
  UASSERT(pos < Size());

  auto& bind = binds_[pos];
  auto& cb = callbacks_[pos];

  if (cb.before_fetch_cb) {
    cb.before_fetch_cb(cb.value, bind);
  }
}

void OutputBindings::AfterFetch(std::size_t pos) {
  UASSERT(pos < Size());

  auto& bind = binds_[pos];
  auto& cb = callbacks_[pos];
  auto& date = dates_[pos];

  if (cb.after_fetch_cb) {
    cb.after_fetch_cb(cb.value, bind, date);
  }
}

std::size_t OutputBindings::Size() const { return binds_.size(); }

bool OutputBindings::Empty() const { return Size() == 0; }

MYSQL_BIND* OutputBindings::GetBindsArray() {
  UASSERT(!Empty());

  return binds_.data();
}

void OutputBindings::Bind(std::size_t pos, uint8_t& val) {
  BindValue(pos, GetNativeType(val), &val, true);
}
void OutputBindings::Bind(std::size_t pos, O<uint8_t>& val) {
  BindPrimitiveOptional(pos, val, true);
}
void OutputBindings::Bind(std::size_t pos, int8_t& val) {
  BindValue(pos, GetNativeType(val), &val);
}
void OutputBindings::Bind(std::size_t pos, O<int8_t>& val) {
  BindPrimitiveOptional(pos, val);
}

void OutputBindings::Bind(std::size_t pos, uint16_t& val) {
  BindValue(pos, GetNativeType(val), &val, true);
}
void OutputBindings::Bind(std::size_t pos, O<uint16_t>& val) {
  BindPrimitiveOptional(pos, val, true);
}
void OutputBindings::Bind(std::size_t pos, int16_t& val) {
  BindValue(pos, GetNativeType(val), &val);
}
void OutputBindings::Bind(std::size_t pos, O<int16_t>& val) {
  BindPrimitiveOptional(pos, val);
}

void OutputBindings::Bind(std::size_t pos, uint32_t& val) {
  BindValue(pos, GetNativeType(val), &val, true);
}
void OutputBindings::Bind(std::size_t pos, O<uint32_t>& val) {
  BindPrimitiveOptional(pos, val, true);
}
void OutputBindings::Bind(std::size_t pos, int32_t& val) {
  BindValue(pos, GetNativeType(val), &val);
}
void OutputBindings::Bind(std::size_t pos, O<int32_t>& val) {
  BindPrimitiveOptional(pos, val);
}

void OutputBindings::Bind(std::size_t pos, uint64_t& val) {
  BindValue(pos, GetNativeType(val), &val, true);
}
void OutputBindings::Bind(std::size_t pos, O<uint64_t>& val) {
  BindPrimitiveOptional(pos, val, true);
}
void OutputBindings::Bind(std::size_t pos, int64_t& val) {
  BindValue(pos, GetNativeType(val), &val);
}
void OutputBindings::Bind(std::size_t pos, O<int64_t>& val) {
  BindPrimitiveOptional(pos, val);
}

void OutputBindings::Bind(std::size_t pos, float& val) {
  BindValue(pos, GetNativeType(val), &val);
}
void OutputBindings::Bind(std::size_t pos, O<float>& val) {
  BindPrimitiveOptional(pos, val);
}
void OutputBindings::Bind(std::size_t pos, double& val) {
  BindValue(pos, GetNativeType(val), &val);
}
void OutputBindings::Bind(std::size_t pos, O<double>& val) {
  BindPrimitiveOptional(pos, val);
}

void OutputBindings::Bind(std::size_t pos, std::string& val) {
  BindString(pos, val);
}
void OutputBindings::Bind(std::size_t pos, O<std::string>& val) {
  BindOptionalString(pos, val);
}

void OutputBindings::Bind(std::size_t pos,
                          std::chrono::system_clock::time_point& val) {
  BindDate(pos, val);
}

void OutputBindings::Bind(std::size_t pos,
                          O<std::chrono::system_clock::time_point>& val) {
  BindOptionalDate(pos, val);
}

void OutputBindings::BindValue(std::size_t pos, enum_field_types type,
                               void* buffer, bool is_unsigned) {
  UASSERT(pos < Size());

  auto& bind = binds_[pos];
  bind.buffer_type = type;
  bind.buffer = buffer;
  bind.buffer_length = 0;
  bind.is_unsigned = is_unsigned;
}

void OutputBindings::BindString(std::size_t pos, std::string& val) {
  UASSERT(pos < Size());

  auto& bind = binds_[pos];
  auto& cb = callbacks_[pos];

  bind.buffer_type = MYSQL_TYPE_STRING;
  bind.buffer = nullptr;
  bind.buffer_length = 0;
  bind.length = &bind.length_value;

  cb.value = &val;
  cb.before_fetch_cb = &StringBeforeFetch;
}

void OutputBindings::StringBeforeFetch(void* value, MYSQL_BIND& bind) {
  auto* string = static_cast<std::string*>(value);
  UASSERT(string);

  // TODO : check length
  string->resize(bind.length_value);
}

void OutputBindings::BindOptionalString(std::size_t pos,
                                        std::optional<std::string>& val) {
  UASSERT(pos < Size());
  UASSERT(!val.has_value());

  auto& bind = binds_[pos];
  auto& cb = callbacks_[pos];

  bind.buffer_type = MYSQL_TYPE_STRING;
  bind.buffer = nullptr;
  bind.buffer_length = 0;
  bind.length = &bind.length_value;

  cb.value = &val;
  cb.before_fetch_cb = &OptionalStringBeforeFetch;
}

void OutputBindings::OptionalStringBeforeFetch(void* value, MYSQL_BIND& bind) {
  auto* optional = static_cast<std::optional<std::string>*>(value);
  UASSERT(optional);

  // TODO : check this
  if (!bind.is_null_value) {
    // TODO : check length
    optional->emplace(bind.length_value, 0);
    bind.buffer = (*optional)->data();
  }
}

void OutputBindings::BindDate(std::size_t pos,
                              std::chrono::system_clock::time_point& val) {
  UASSERT(pos < Size());

  auto& date = dates_[pos];
  auto& bind = binds_[pos];
  auto& cb = callbacks_[pos];

  bind.buffer_type = MYSQL_TYPE_DATETIME;
  bind.buffer = &date;
  bind.buffer_length = sizeof(MYSQL_TIME);

  cb.value = &val;
  cb.after_fetch_cb = &DateAfterFetch;
}

void OutputBindings::DateAfterFetch(void* value, MYSQL_BIND&,
                                    MYSQL_TIME& date) {
  auto* timepoint = static_cast<std::chrono::system_clock::time_point*>(value);
  UASSERT(timepoint);

  const date::year year(date.year);
  const date::month month{date.month};
  const date::day day{date.day};
  const std::chrono::hours hour{date.hour};
  const std::chrono::minutes minute{date.minute};
  const std::chrono::seconds second{date.second};
  const MariaDBTimepointResolution microsecond{date.second_part};

  *timepoint =
      date::sys_days{year / month / day} + hour + minute + second + microsecond;
}

void OutputBindings::BindOptionalDate(
    std::size_t pos,
    std::optional<std::chrono::system_clock::time_point>& val) {
  UASSERT(pos < Size());
  UASSERT(!val.has_value());

  auto& date = dates_[pos];
  auto& bind = binds_[pos];
  auto& cb = callbacks_[pos];

  bind.buffer_type = MYSQL_TYPE_DATETIME;
  bind.buffer = &date;
  bind.buffer_length = sizeof(MYSQL_TIME);

  cb.value = &val;
  cb.after_fetch_cb = &OptionalDateAfterFetch;
}

void OutputBindings::OptionalDateAfterFetch(void* value, MYSQL_BIND& bind,
                                            MYSQL_TIME& date) {
  auto* optional =
      static_cast<std::optional<std::chrono::system_clock::time_point>*>(value);
  UASSERT(optional);

  // TODO : check this
  if (!bind.is_null_value) {
    OutputBindings::DateAfterFetch(&*optional, bind, date);
  }
}

}  // namespace storages::mysql::impl::bindings

USERVER_NAMESPACE_END
