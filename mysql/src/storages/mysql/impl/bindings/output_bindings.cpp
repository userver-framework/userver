#include <storages/mysql/impl/bindings/output_bindings.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::bindings {

namespace {

bool IsNumericBindable(enum_field_types bind_type,
                       enum_field_types field_type) {
  UASSERT(NativeBindsHelper::IsFieldNumeric(bind_type) &&
          NativeBindsHelper::IsFieldNumeric(field_type));

  return NativeBindsHelper::NumericFieldWidth(bind_type) >=
         NativeBindsHelper::NumericFieldWidth(field_type);
}

bool IsBlob(enum_field_types field_type) {
  return field_type == MYSQL_TYPE_BLOB || field_type == MYSQL_TYPE_TINY_BLOB ||
         field_type == MYSQL_TYPE_MEDIUM_BLOB ||
         field_type == MYSQL_TYPE_LONG_BLOB;
}

bool IsBindable(enum_field_types bind_type, enum_field_types field_type) {
  if (bind_type == MYSQL_TYPE_STRING) {
    return field_type == MYSQL_TYPE_STRING ||
           field_type == MYSQL_TYPE_VAR_STRING ||
           field_type == MYSQL_TYPE_VARCHAR || IsBlob(field_type);
  }

  if (bind_type == MYSQL_TYPE_DECIMAL) {
    return field_type == MYSQL_TYPE_DECIMAL ||
           field_type == MYSQL_TYPE_NEWDECIMAL;
  }

  if (bind_type == MYSQL_TYPE_JSON) {
    return field_type == MYSQL_TYPE_JSON || IsBlob(field_type);
  }

  if (bind_type == MYSQL_TYPE_DATE) {
    return field_type == MYSQL_TYPE_DATE || field_type == MYSQL_TYPE_NEWDATE;
  }

  if (bind_type == MYSQL_TYPE_DATETIME) {
    return field_type == MYSQL_TYPE_DATETIME;
  }

  if (bind_type == MYSQL_TYPE_TIMESTAMP) {
    return field_type == MYSQL_TYPE_TIMESTAMP ||
           // TODO : think about this: it's convenient to use, but narrowing
           field_type == MYSQL_TYPE_DATE || field_type == MYSQL_TYPE_NEWDATE ||
           field_type == MYSQL_TYPE_DATETIME;
  }

  if (NativeBindsHelper::IsFieldNumeric(bind_type) &&
      NativeBindsHelper::IsFieldNumeric(field_type)) {
    return IsNumericBindable(bind_type, field_type);
  }

  // TODO : MYSQL_TYPE_BIT
  return bind_type == field_type;
}

}  // namespace

OutputBindings::OutputBindings(std::size_t size)
    : owned_binds_{size}, binds_ptr_{owned_binds_.data()} {
  // Not always necessary, but we can live with that
  callbacks_.resize(size);
  intermediate_buffers_.resize(size);
}

OutputBindings::OutputBindings(OutputBindings&& other) noexcept
    : callbacks_{std::move(other.callbacks_)},
      intermediate_buffers_{std::move(other.intermediate_buffers_)},
      owned_binds_{std::move(other.owned_binds_)},
      // Technically this is incorrect, since bind_ptr_ could've been updated to
      // point into libmariadb, but move neven happens after that
      binds_ptr_{owned_binds_.data()} {}

void OutputBindings::BeforeFetch(std::size_t pos) {
  UASSERT(pos < Size());

  auto& bind = GetBind(pos);
  auto& cb = callbacks_[pos];
  auto& intermediate_buffer = intermediate_buffers_[pos];

  if (cb.before_fetch_cb) {
    cb.before_fetch_cb(cb.value, bind, intermediate_buffer);
  }
}

void OutputBindings::AfterFetch(std::size_t pos) {
  UASSERT(pos < Size());

  auto& bind = GetBind(pos);
  auto& cb = callbacks_[pos];
  auto& buffer = intermediate_buffers_[pos];

  if (cb.after_fetch_cb) {
    cb.after_fetch_cb(cb.value, bind, buffer);
  }
}

std::size_t OutputBindings::Size() const { return owned_binds_.size(); }

bool OutputBindings::Empty() const { return Size() == 0; }

MYSQL_BIND* OutputBindings::GetBindsArray() {
  UASSERT(!Empty());

  return binds_ptr_;
}

void OutputBindings::WrapBinds(void* binds_array) {
  UASSERT(binds_array);
  binds_ptr_ = static_cast<MYSQL_BIND*>(binds_array);
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

void OutputBindings::Bind(std::size_t pos, io::DecimalWrapper& val) {
  BindDecimal(pos, val);
}
void OutputBindings::Bind(std::size_t pos, O<io::DecimalWrapper>& val) {
  BindOptionalDecimal(pos, val);
}

void OutputBindings::Bind(std::size_t pos, std::string& val) {
  BindString(pos, val);
}
void OutputBindings::Bind(std::size_t pos, O<std::string>& val) {
  BindOptionalString(pos, val);
}

void OutputBindings::Bind(std::size_t pos, formats::json::Value& val) {
  BindJson(pos, val);
}
void OutputBindings::Bind(std::size_t pos, O<formats::json::Value>& val) {
  BindOptionalJson(pos, val);
}

void OutputBindings::Bind(std::size_t pos,
                          std::chrono::system_clock::time_point& val) {
  BindTimePoint(pos, val);
}
void OutputBindings::Bind(std::size_t pos,
                          O<std::chrono::system_clock::time_point>& val) {
  BindOptionalTimePoint(pos, val);
}
void OutputBindings::Bind(std::size_t pos, Date& val) { BindDate(pos, val); }
void OutputBindings::Bind(std::size_t pos, O<Date>& val) {
  BindOptionalDate(pos, val);
}
void OutputBindings::Bind(std::size_t pos, DateTime& val) {
  BindDateTime(pos, val);
}
void OutputBindings::Bind(std::size_t pos, O<DateTime>& val) {
  BindOptionalDateTime(pos, val);
}

void OutputBindings::BindValue(std::size_t pos, enum_field_types type,
                               void* buffer, bool is_unsigned) {
  auto& bind = GetBind(pos);
  bind.buffer_type = type;
  bind.buffer = buffer;
  // bind.buffer_length = 0;
  bind.is_unsigned = is_unsigned;
}

void OutputBindings::BindString(std::size_t pos, std::string& val) {
  auto& bind = GetBind(pos);
  auto& cb = callbacks_[pos];

  bind.buffer_type = MYSQL_TYPE_STRING;
  bind.buffer = nullptr;
  bind.buffer_length = 0;
  bind.length = &bind.length_value;

  cb.value = &val;
  cb.before_fetch_cb = &StringBeforeFetch;
}

void OutputBindings::StringBeforeFetch(void* value, MYSQL_BIND& bind,
                                       FieldIntermediateBuffer&) {
  auto* string = static_cast<std::string*>(value);
  UASSERT(string);

  string->resize(bind.length_value);
  bind.buffer = string->data();
  bind.buffer_length = bind.length_value;
}

void OutputBindings::BindOptionalString(std::size_t pos,
                                        std::optional<std::string>& val) {
  UASSERT(!val.has_value());

  auto& bind = GetBind(pos);
  auto& cb = callbacks_[pos];

  bind.buffer_type = MYSQL_TYPE_STRING;
  bind.buffer = nullptr;
  bind.buffer_length = 0;
  bind.length = &bind.length_value;
  bind.is_null = &bind.is_null_value;
  bind.error = &bind.error_value;

  cb.value = &val;
  cb.before_fetch_cb = &OptionalStringBeforeFetch;
}

void OutputBindings::OptionalStringBeforeFetch(void* value, MYSQL_BIND& bind,
                                               FieldIntermediateBuffer&) {
  auto* optional = static_cast<std::optional<std::string>*>(value);
  UASSERT(optional);

  if (!bind.is_null_value) {
    optional->emplace(bind.length_value, 0);
    bind.buffer = (*optional)->data();
    bind.buffer_length = bind.length_value;
  }
}

void OutputBindings::BindTimePoint(std::size_t pos,
                                   std::chrono::system_clock::time_point& val) {
  auto& date = intermediate_buffers_[pos].time;
  auto& bind = GetBind(pos);
  auto& cb = callbacks_[pos];

  bind.buffer_type = MYSQL_TYPE_TIMESTAMP;
  bind.buffer = &date;
  bind.buffer_length = sizeof(MYSQL_TIME);

  cb.value = &val;
  cb.after_fetch_cb = &TimepointAfterFetch;
}

void OutputBindings::TimepointAfterFetch(void* value, MYSQL_BIND&,
                                         FieldIntermediateBuffer& buffer) {
  auto* timepoint = static_cast<std::chrono::system_clock::time_point*>(value);
  UASSERT(timepoint);

  *timepoint = FromNativeTime(buffer.time);
}

void OutputBindings::BindOptionalTimePoint(
    std::size_t pos,
    std::optional<std::chrono::system_clock::time_point>& val) {
  UASSERT(!val.has_value());

  auto& date = intermediate_buffers_[pos].time;
  auto& bind = GetBind(pos);
  auto& cb = callbacks_[pos];

  bind.buffer_type = MYSQL_TYPE_TIMESTAMP;
  bind.buffer = &date;
  bind.buffer_length = sizeof(MYSQL_TIME);
  bind.is_null = &bind.is_null_value;

  cb.value = &val;
  cb.after_fetch_cb = &OptionalTimePointAfterFetch;
}

void OutputBindings::OptionalTimePointAfterFetch(
    void* value, MYSQL_BIND& bind, FieldIntermediateBuffer& buffer) {
  auto* optional =
      static_cast<std::optional<std::chrono::system_clock::time_point>*>(value);
  UASSERT(optional);

  if (!bind.is_null_value) {
    optional->emplace();
    TimepointAfterFetch(&*optional, bind, buffer);
  }
}

void OutputBindings::BindDate(std::size_t pos, Date& val) {
  auto& date = intermediate_buffers_[pos].time;
  auto& bind = GetBind(pos);
  auto& cb = callbacks_[pos];

  bind.buffer_type = MYSQL_TYPE_DATE;
  bind.buffer = &date;
  bind.buffer_length = sizeof(MYSQL_TIME);

  cb.value = &val;
  cb.after_fetch_cb = &DateAfterFetch;
}

void OutputBindings::DateAfterFetch(void* value, MYSQL_BIND&,
                                    FieldIntermediateBuffer& buffer) {
  auto* date = static_cast<Date*>(value);
  UASSERT(date);

  auto& native_time = buffer.time;

  *date = Date{native_time.year, native_time.month, native_time.day};
}

void OutputBindings::BindOptionalDate(std::size_t pos,
                                      std::optional<Date>& val) {
  UASSERT(!val.has_value());

  auto& date = intermediate_buffers_[pos].time;
  auto& bind = GetBind(pos);
  auto& cb = callbacks_[pos];

  bind.buffer_type = MYSQL_TYPE_DATE;
  bind.buffer = &date;
  bind.buffer_length = sizeof(MYSQL_TIME);
  bind.is_null = &bind.is_null_value;

  cb.value = &val;
  cb.after_fetch_cb = &OptionalDateAfterFetch;
}

void OutputBindings::OptionalDateAfterFetch(void* value, MYSQL_BIND& bind,
                                            FieldIntermediateBuffer& buffer) {
  auto* optional = static_cast<std::optional<Date>*>(value);
  UASSERT(optional);

  if (!bind.is_null_value) {
    optional->emplace();
    DateAfterFetch(&*optional, bind, buffer);
  }
}

void OutputBindings::BindDateTime(std::size_t pos, DateTime& val) {
  auto& date = intermediate_buffers_[pos].time;
  auto& bind = GetBind(pos);
  auto& cb = callbacks_[pos];

  bind.buffer_type = MYSQL_TYPE_DATETIME;
  bind.buffer = &date;
  bind.buffer_length = sizeof(MYSQL_TIME);

  cb.value = &val;
  cb.after_fetch_cb = &DateTimeAfterFetch;
}

void OutputBindings::DateTimeAfterFetch(void* value, MYSQL_BIND&,
                                        FieldIntermediateBuffer& buffer) {
  auto* datetime = static_cast<DateTime*>(value);
  UASSERT(datetime);

  auto& native_time = buffer.time;

  *datetime =
      DateTime{native_time.year,       native_time.month,  native_time.day,
               native_time.hour,       native_time.minute, native_time.second,
               native_time.second_part};
}

void OutputBindings::BindOptionalDateTime(std::size_t pos,
                                          std::optional<DateTime>& val) {
  UASSERT(!val.has_value());

  auto& date = intermediate_buffers_[pos].time;
  auto& bind = GetBind(pos);
  auto& cb = callbacks_[pos];

  bind.buffer_type = MYSQL_TYPE_DATETIME;
  bind.buffer = &date;
  bind.buffer_length = sizeof(MYSQL_TIME);
  bind.is_null = &bind.is_null_value;

  cb.value = &val;
  cb.after_fetch_cb = &OptionalDateTimeAfterFetch;
}

void OutputBindings::OptionalDateTimeAfterFetch(
    void* value, MYSQL_BIND& bind, FieldIntermediateBuffer& buffer) {
  auto* optional = static_cast<std::optional<DateTime>*>(value);
  UASSERT(optional);

  if (!bind.is_null_value) {
    optional->emplace();
    DateTimeAfterFetch(&*optional, bind, buffer);
  }
}

void OutputBindings::BindJson(std::size_t pos, formats::json::Value& val) {
  auto& bind = GetBind(pos);
  auto& cb = callbacks_[pos];

  bind.buffer_type = MYSQL_TYPE_JSON;
  bind.buffer = nullptr;
  bind.buffer_length = 0;
  bind.length = &bind.length_value;

  cb.value = &val;
  cb.before_fetch_cb = &JsonBeforeFetch;
  cb.after_fetch_cb = &JsonAfterFetch;
}

void OutputBindings::JsonBeforeFetch(void*, MYSQL_BIND& bind,
                                     FieldIntermediateBuffer& buffer) {
  auto& string = buffer.string;

  string.resize(bind.length_value);
  bind.buffer = string.data();
  bind.buffer_length = bind.length_value;
}

void OutputBindings::JsonAfterFetch(void* value, MYSQL_BIND&,
                                    FieldIntermediateBuffer& buffer) {
  auto* json = static_cast<formats::json::Value*>(value);
  UASSERT(json);

  *json = formats::json::FromString(buffer.string);
}

void OutputBindings::BindOptionalJson(
    std::size_t pos, std::optional<formats::json::Value>& val) {
  UASSERT(!val.has_value());

  auto& bind = GetBind(pos);
  auto& cb = callbacks_[pos];

  bind.buffer_type = MYSQL_TYPE_JSON;
  bind.buffer = nullptr;
  bind.buffer_length = 0;
  bind.length = &bind.length_value;
  bind.is_null = &bind.is_null_value;

  cb.value = &val;
  cb.before_fetch_cb = &OptionalJsonBeforeFetch;
  cb.after_fetch_cb = &OptionalJsonAfterFetch;
}

void OutputBindings::OptionalJsonBeforeFetch(void*, MYSQL_BIND& bind,
                                             FieldIntermediateBuffer& buffer) {
  auto& string = buffer.string;
  if (!bind.is_null_value) {
    string.resize(bind.length_value);
    bind.buffer = string.data();
    bind.buffer_length = bind.length_value;
  }
}

void OutputBindings::OptionalJsonAfterFetch(void* value, MYSQL_BIND& bind,
                                            FieldIntermediateBuffer& buffer) {
  auto* optional = static_cast<std::optional<formats::json::Value>*>(value);
  UASSERT(optional);

  if (!bind.is_null_value) {
    optional->emplace();
    JsonAfterFetch(&*optional, bind, buffer);
  }
}

void OutputBindings::BindDecimal(std::size_t pos, io::DecimalWrapper& val) {
  auto& bind = GetBind(pos);
  auto& cb = callbacks_[pos];

  // !important: val is a reference to temporary, copy it
  auto& buffer = intermediate_buffers_[pos];
  buffer.decimal = val;

  bind.buffer_type = MYSQL_TYPE_DECIMAL;
  bind.buffer = nullptr;
  bind.buffer_length = 0;
  bind.length = &bind.length_value;

  cb.value = &buffer.decimal;
  cb.before_fetch_cb = &DecimalBeforeFetch;
  cb.after_fetch_cb = &DecimalAfterFetch;
}

void OutputBindings::DecimalBeforeFetch(void*, MYSQL_BIND& bind,
                                        FieldIntermediateBuffer& buffer) {
  auto& string = buffer.string;

  string.resize(bind.length_value);
  bind.buffer = string.data();
  bind.buffer_length = bind.length_value;
}

void OutputBindings::DecimalAfterFetch(void* value, MYSQL_BIND&,
                                       FieldIntermediateBuffer& buffer) {
  auto* wrapper = static_cast<io::DecimalWrapper*>(value);
  UASSERT(wrapper);

  wrapper->Restore(buffer.string);
}

void OutputBindings::BindOptionalDecimal(std::size_t pos,
                                         O<io::DecimalWrapper>& val) {
  // We assert that it HAS value, because it always has, it's optional itself to
  // distinguish easier. What's WRAPPED is actually optional
  UASSERT(val.has_value());

  // !important: val is a reference to temporary, copy it
  auto& buffer = intermediate_buffers_[pos];
  buffer.decimal = *val;

  auto& bind = GetBind(pos);
  auto& cb = callbacks_[pos];

  bind.buffer_type = MYSQL_TYPE_DECIMAL;
  bind.buffer = nullptr;
  bind.buffer_length = 0;
  bind.length = &bind.length_value;
  bind.is_null = &bind.is_null_value;

  cb.value = &buffer.decimal;
  cb.before_fetch_cb = &OptionalDecimalBeforeFetch;
  cb.after_fetch_cb = &OptionalDecimalAfterFetch;
}

void OutputBindings::OptionalDecimalBeforeFetch(
    void*, MYSQL_BIND& bind, FieldIntermediateBuffer& buffer) {
  auto& string = buffer.string;
  if (!bind.is_null_value) {
    string.resize(bind.length_value);
    bind.buffer = string.data();
    bind.buffer_length = bind.length_value;
  }
}

void OutputBindings::OptionalDecimalAfterFetch(
    void* value, MYSQL_BIND& bind, FieldIntermediateBuffer& buffer) {
  auto* decimal = static_cast<io::DecimalWrapper*>(value);
  UASSERT(decimal);

  if (!bind.is_null_value) {
    DecimalAfterFetch(decimal, bind, buffer);
  }
}

void OutputBindings::ValidateAgainstStatement(MYSQL_STMT& statement) {
  const auto fields_count = mysql_stmt_field_count(&statement);
  UINVARIANT(fields_count == Size(),
             fmt::format("Fields count mismatch: expected {}, got {}",
                         fields_count, Size()));
  if (fields_count == 0) {
    return;
  }

  const auto res_deleter = [](MYSQL_RES* res) { mysql_free_result(res); };
  const std::unique_ptr<MYSQL_RES, decltype(res_deleter)> result_metadata{
      mysql_stmt_result_metadata(&statement), res_deleter};
  UASSERT(result_metadata);

  const auto* fields = mysql_fetch_fields(result_metadata.get());
  for (std::size_t i = 0; i < fields_count; ++i) {
    const auto& bind = GetBind(i);
    const auto& field = fields[i];

    ValidateBind(i, bind, field);
  }
}

MYSQL_BIND& OutputBindings::GetBind(std::size_t pos) {
  UASSERT(pos < Size());
  auto& bind = binds_ptr_[pos];
  bind.error = &bind.error_value;

  return bind;
}

void OutputBindings::ValidateBind(std::size_t pos, const MYSQL_BIND& bind,
                                  const MYSQL_FIELD& field) {
  const auto error_for_column = [&field, pos](std::string_view error,
                                              const auto&... args) {
    return fmt::format("Field type mismatch for column '{}' ({}-th column): {}",
                       field.name, pos,
                       fmt::format(fmt::runtime(error), args...));
  };

  // validate general types match
  UINVARIANT(IsBindable(bind.buffer_type, field.type),
             error_for_column("expected {}, got {}",
                              NativeTypeToString(bind.buffer_type),
                              NativeTypeToString(field.type)));

  // validate null/not null
  const bool is_bind_nullable = bind.is_null != nullptr;
  const bool is_field_nullable = !(field.flags & NOT_NULL_FLAG);
  /*
   * SN = server nullable, CN = client nullable
   * SN    |  CN   |  OK
   * true  | true  | true
   * true  | false | false
   * false | true  | true
   * false | false | true
   */
  UINVARIANT(!(is_field_nullable && !is_bind_nullable),
             error_for_column(
                 "field is nullable in DB but not-nullable in client code"));

  const auto signed_to_string = [](bool is_nullable) -> std::string_view {
    return is_nullable ? "signed" : "unsigned";
  };
  // validate signed/unsigned for numeric types
  if (IsFieldNumeric(field.type)) {
    const bool is_bind_signed = !bind.is_unsigned;
    const bool is_field_signed = !(field.flags & UNSIGNED_FLAG);
    UINVARIANT(is_bind_signed == is_field_signed,
               error_for_column("field is {} in DB but {} in client code",
                                signed_to_string(is_field_signed),
                                signed_to_string(is_bind_signed)));
  }

  // TODO : validate decimal somehow
}

}  // namespace storages::mysql::impl::bindings

USERVER_NAMESPACE_END
