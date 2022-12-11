#include <storages/mysql/impl/mysql_binds_storage.hpp>

#include <date.h>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

BindsStorage::BindsStorage(BindsType binds_type) : binds_type_{binds_type} {}

BindsStorage::~BindsStorage() = default;

BindsStorage::At::At(BindsStorage& storage, std::size_t pos)
    : storage_{storage}, pos_{pos} {}

void BindsStorage::At::Bind(std::uint8_t& val) {
  storage_.DoBind(pos_, MYSQL_TYPE_TINY, &val, 0, true);
}

void BindsStorage::At::Bind(O<std::uint8_t>& val) { BindOptional(val); }

void BindsStorage::At::Bind(std::int8_t& val) {
  storage_.DoBind(pos_, MYSQL_TYPE_TINY, &val, 0);
}

void BindsStorage::At::Bind(O<std::int8_t>& val) { BindOptional(val); }

void BindsStorage::At::Bind(std::uint16_t& val) {
  storage_.DoBind(pos_, MYSQL_TYPE_SHORT, &val, 0, true);
}

void BindsStorage::At::Bind(O<std::uint16_t>& val) { BindOptional(val); }

void BindsStorage::At::Bind(std::int16_t& val) {
  storage_.DoBind(pos_, MYSQL_TYPE_SHORT, &val, 0);
}

void BindsStorage::At::Bind(O<std::int16_t>& val) { BindOptional(val); }

void BindsStorage::At::Bind(std::uint32_t& val) {
  storage_.DoBind(pos_, MYSQL_TYPE_LONG, &val, 0, true);
}

void BindsStorage::At::Bind(O<std::uint32_t>& val) { BindOptional(val); }

void BindsStorage::At::Bind(std::int32_t& val) {
  storage_.DoBind(pos_, MYSQL_TYPE_LONG, &val, 0);
}

void BindsStorage::At::Bind(O<std::int32_t>& val) { BindOptional(val); }

void BindsStorage::At::Bind(std::uint64_t& val) {
  storage_.DoBind(pos_, MYSQL_TYPE_LONGLONG, &val, 0, true);
}

void BindsStorage::At::Bind(O<std::uint64_t>& val) { BindOptional(val); }

void BindsStorage::At::Bind(std::int64_t& val) {
  storage_.DoBind(pos_, MYSQL_TYPE_LONGLONG, &val, 0);
}

void BindsStorage::At::Bind(O<std::int64_t>& val) { BindOptional(val); }

void BindsStorage::At::Bind(float& val) {
  storage_.DoBind(pos_, MYSQL_TYPE_FLOAT, &val, 0);
}

void BindsStorage::At::Bind(O<float>& val) { BindOptional(val); }

void BindsStorage::At::Bind(double& val) {
  storage_.DoBind(pos_, MYSQL_TYPE_DOUBLE, &val, 0);
}

void BindsStorage::At::Bind(O<double>& val) { BindOptional(val); }

void BindsStorage::At::Bind(std::string& val) {
  if (storage_.binds_type_ == BindsType::kParameters) {
    storage_.DoBind(pos_, MYSQL_TYPE_STRING, val.data(), val.length());
  } else {
    storage_.DoBindOutputString(pos_, val);
  }
}

void BindsStorage::At::Bind(O<std::string>& val) { BindOptional(val); }

void BindsStorage::At::Bind(std::string_view& val) {
  UINVARIANT(storage_.binds_type_ != BindsType::kResult,
             "std::string_view is not supported in result extraction");

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  storage_.DoBind(pos_, MYSQL_TYPE_STRING, const_cast<char*>(val.data()),
                  val.length());
}

void BindsStorage::At::Bind(std::chrono::system_clock::time_point& val) {
  if (storage_.binds_type_ == BindsType::kParameters) {
    storage_.DoBindDate(pos_, val);
  } else {
    storage_.DoBindOutputDate(pos_, val);
  }
}

void BindsStorage::At::Bind(O<std::chrono::system_clock::time_point>& val) {
  BindOptional(val);
}

void BindsStorage::At::DoBindNull() {
  storage_.DoBind(pos_, MYSQL_TYPE_NULL, nullptr, 0);
}

BindsStorage::At BindsStorage::GetAt(std::size_t pos) { return At{*this, pos}; }

void BindsStorage::UpdateBeforeFetch(std::size_t pos) {
  auto& bind = binds_[pos];
  if (bind.buffer_type == MYSQL_TYPE_STRING) {
    ResizeOutputString(pos);
  } else if (!output_optionals_.empty()) {
    auto& opt = output_optionals_[pos];
    if (opt.optional && !bind.is_null_value) {
      opt.emplace_cb(&opt, opt.optional);
      opt.fix_bind_cb(&opt, &bind);

      if (bind.buffer_type != MYSQL_TYPE_DATETIME) {
        bind.buffer = opt.get_data_ptr(opt.optional);
      } else {
        wrapped_dates_[pos].UpdateOutput(opt.get_data_ptr(opt.optional));
      }
    }

    int a = 5;
  }
}

void BindsStorage::UpdateAfterFetch(std::size_t pos) {
  auto& bind = binds_[pos];
  if (bind.is_null && *bind.is_null) {
    UASSERT(pos <= output_optionals_.size());
    output_optionals_[pos].reset_cb(output_optionals_[pos].optional);
  } else {
    if (bind.buffer_type == MYSQL_TYPE_DATETIME) {
      FillOutputDate(pos);
    }
  }
}

void BindsStorage::ResizeOutputString(std::size_t pos) {
  UASSERT(binds_type_ == BindsType::kResult);

  auto& output_string = output_strings_[pos];
  auto& bind = binds_[pos];

  output_string.output->resize(output_string.output_length);
  bind.buffer = output_string.output->data();
  bind.buffer_length = output_string.output->size();
}

MYSQL_BIND* BindsStorage::GetBindsArray() { return binds_.data(); }

std::size_t BindsStorage::Size() const { return binds_.size(); }

bool BindsStorage::Empty() const { return binds_.empty(); }

void BindsStorage::SetBindCallbacks(void* user_data,
                                    void (*param_cb)(void*, void*,
                                                     std::size_t)) {
  user_data_ = user_data;
  param_cb_ = param_cb;
}

void* BindsStorage::GetUserData() const { return user_data_; }

void* BindsStorage::GetParamCb() const {
  return reinterpret_cast<void*>(param_cb_);
}

void BindsStorage::ResizeIfNecessary(std::size_t pos) {
  if (pos >= binds_.size()) {
    const auto new_size = pos + 1;
    binds_.resize(new_size);
    wrapped_dates_.resize(new_size);

    if (binds_type_ == BindsType::kResult) {
      output_strings_.resize(new_size);
      output_optionals_.resize(new_size);
    }
  }
}

void BindsStorage::DoBindOutputString(std::size_t pos, std::string& val) {
  ResizeIfNecessary(pos);

  auto& output_string = output_strings_[pos];
  output_string.output = &val;

  auto& bind = binds_[pos];
  bind.buffer_type = MYSQL_TYPE_STRING;
  bind.buffer = nullptr;
  bind.buffer_length = 0;
  bind.length = &output_string.output_length;
}

void BindsStorage::DoBindDate(std::size_t pos,
                              std::chrono::system_clock::time_point& val) {
  ResizeIfNecessary(pos);

  // https://stackoverflow.com/a/15958113/10508079
  auto dp = date::floor<date::days>(val);
  auto ymd = date::year_month_day{date::floor<date::days>(val)};
  auto time = date::make_time(
      std::chrono::duration_cast<MariaDBTimepointResolution>(val - dp));

  auto& native_time = wrapped_dates_[pos].native_time;
  native_time.year = ymd.year().operator int();
  native_time.month = ymd.month().operator unsigned int();
  native_time.day = ymd.day().operator unsigned int();
  native_time.hour = time.hours().count();
  native_time.minute = time.minutes().count();
  native_time.second = time.seconds().count();
  native_time.second_part = time.subseconds().count();

  native_time.time_type = MYSQL_TIMESTAMP_DATETIME;

  DoBind(pos, MYSQL_TYPE_DATETIME, &native_time, sizeof(MYSQL_TIME));
}

void BindsStorage::DoBindOutputDate(
    std::size_t pos, std::chrono::system_clock::time_point& val) {
  ResizeIfNecessary(pos);

  auto& bind = binds_[pos];
  auto& output = wrapped_dates_[pos];

  output.output = &val;
  bind.buffer_type = MYSQL_TYPE_DATETIME;
  bind.buffer = &output.native_time;
  bind.buffer_length = sizeof(MYSQL_TIME);
}

void BindsStorage::FillOutputDate(std::size_t pos) {
  auto& output = wrapped_dates_[pos];

  auto& native_time = output.native_time;
  auto& output_tp = *output.output;

  const date::year year(native_time.year);
  const date::month month{native_time.month};
  const date::day day{native_time.day};
  const std::chrono::hours hour{native_time.hour};
  const std::chrono::minutes minute{native_time.minute};
  const std::chrono::seconds second{native_time.second};
  const MariaDBTimepointResolution microsecond{native_time.second_part};

  output_tp =
      date::sys_days{year / month / day} + hour + minute + second + microsecond;
}

void BindsStorage::DoBind(std::size_t pos, enum_field_types type, void* buffer,
                          std::size_t length, bool is_unsigned) {
  ResizeIfNecessary(pos);

  auto& bind = binds_[pos];
  bind.buffer_type = type;
  bind.buffer = buffer;
  bind.buffer_length = length;
  bind.is_unsigned = is_unsigned;

  if (binds_type_ == BindsType::kResult) {
    // UpdateBindBufferLength(bind, type);
  }
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
