#pragma once

#include <storages/mysql/impl/bindings/binds_storage_interface.hpp>

#include <vector>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::bindings {

class OutputBindings final : public BindsStorageInterface<false> {
 public:
  OutputBindings(std::size_t size);

  void BeforeFetch(std::size_t pos);
  void AfterFetch(std::size_t pos);

  std::size_t Size() const final;
  bool Empty() const final;
  MYSQL_BIND* GetBindsArray() final;

  void Bind(std::size_t pos, std::uint8_t& val) final;
  void Bind(std::size_t pos, O<std::uint8_t>& val) final;
  void Bind(std::size_t pos, std::int8_t& val) final;
  void Bind(std::size_t pos, O<std::int8_t>& val) final;

  void Bind(std::size_t pos, std::uint16_t& val) final;
  void Bind(std::size_t pos, O<std::uint16_t>& val) final;
  void Bind(std::size_t pos, std::int16_t& val) final;
  void Bind(std::size_t pos, O<std::int16_t>& val) final;

  void Bind(std::size_t pos, std::uint32_t& val) final;
  void Bind(std::size_t pos, O<std::uint32_t>& val) final;
  void Bind(std::size_t pos, std::int32_t& val) final;
  void Bind(std::size_t pos, O<std::int32_t>& val) final;

  void Bind(std::size_t pos, std::uint64_t& val) final;
  void Bind(std::size_t pos, O<std::uint64_t>& val) final;
  void Bind(std::size_t pos, std::int64_t& val) final;
  void Bind(std::size_t pos, O<std::int64_t>& val) final;

  void Bind(std::size_t pos, float& val) final;
  void Bind(std::size_t pos, O<float>& val) final;
  void Bind(std::size_t pos, double& val) final;
  void Bind(std::size_t pos, O<double>& val) final;

  void Bind(std::size_t pos, std::string& val) final;
  void Bind(std::size_t pos, O<std::string>& val) final;

  void Bind(std::size_t, std::string_view&) final {
    UINVARIANT(false, "string_view in output params is not supported");
  }
  void Bind(std::size_t, O<std::string_view>&) final {
    UINVARIANT(false, "string_view in output params is not supported");
  }

  void Bind(std::size_t pos, std::chrono::system_clock::time_point& val) final;
  void Bind(std::size_t pos,
            O<std::chrono::system_clock::time_point>& val) final;

  void ValidateAgainstStatement(MYSQL_STMT& statement);

 private:
  // The special problem of binding primitive optionals: we don't know whether
  // it contains a value upfront, so after mysql_stmt_fetch we have to determine
  // it and if it does emplace the optional and fix the buffer before
  // mysql_stmt_fetch_column.
  //
  // BUT! mariadb doesn't care and for primitive types just writes into the
  // provided buffer (which is null obviously), so instead we emplace the
  // optional at bind time and reset it before mysql_stmt_fetch_column (or
  // after, doesn't matter)
  template <typename T>
  void BindPrimitiveOptional(std::size_t pos, std::optional<T>& val,
                             bool is_unsigned = false);
  template <typename T>
  static void PrimitiveOptionalBeforeFetch(void* value, MYSQL_BIND& bind);

  void BindValue(std::size_t pos, enum_field_types type, void* buffer,
                 bool is_unsigned = false);

  // The special problem of binding strings: we don't know the size needed
  // upfront, so after mysql_stmt_fetch we have MYSQL_TRUNCATED and length set,
  // now we have to resize the string accordingly before mysql_stmt_fetch_column
  void BindString(std::size_t pos, std::string& val);
  static void StringBeforeFetch(void* value, MYSQL_BIND& bind);

  // The special problem of binding optional strings: same as for strings, but
  // now we have to determine whether it contains a value first, and if it does
  // emplace the optional and resize its string
  void BindOptionalString(std::size_t pos, std::optional<std::string>& val);
  static void OptionalStringBeforeFetch(void* value, MYSQL_BIND& bind);

  // The special problem of binding dates: they are not buffer-wise
  // compatible with C++ types, so we have to store DB date in some
  // storage first, call mysql_stmt_fetch_column and fill the C++ type after
  // that
  void BindDate(std::size_t pos, std::chrono::system_clock::time_point& val);
  static void DateAfterFetch(void* value, MYSQL_BIND& bind, MYSQL_TIME& date);

  // The special problem of binding optional dates: same as for dates, but have
  // to emplace the optional first if a value is present
  void BindOptionalDate(
      std::size_t pos,
      std::optional<std::chrono::system_clock::time_point>& val);
  static void OptionalDateAfterFetch(void* value, MYSQL_BIND& bind,
                                     MYSQL_TIME& date);

  static void ValidateBind(std::size_t pos, const MYSQL_BIND& bind,
                           const MYSQL_FIELD& field);

  struct BindCallbacks final {
    using BeforeFetchCb = void (*)(void* value, MYSQL_BIND& bind);
    using AfterFetchCb = void (*)(void* value, MYSQL_BIND& bind,
                                  MYSQL_TIME& date);

    void* value{nullptr};
    BeforeFetchCb before_fetch_cb{nullptr};
    AfterFetchCb after_fetch_cb{nullptr};
  };

  std::vector<BindCallbacks> callbacks_;
  std::vector<MYSQL_TIME> dates_;
  std::vector<MYSQL_BIND> binds_;
};

template <typename T>
void OutputBindings::BindPrimitiveOptional(std::size_t pos,
                                           std::optional<T>& val,
                                           bool is_unsigned) {
  UASSERT(pos < Size());
  UASSERT(!val.has_value());

  auto& bind = binds_[pos];
  auto& cb = callbacks_[pos];

  val.emplace();
  bind.buffer_type = GetNativeType<T>();
  bind.buffer = &*val;
  bind.buffer_length = 0;
  bind.is_unsigned = is_unsigned;
  bind.is_null = &bind.is_null_value;

  cb.value = &val;
  cb.before_fetch_cb = &PrimitiveOptionalBeforeFetch<T>;
}

template <typename T>
void OutputBindings::PrimitiveOptionalBeforeFetch(void* value,
                                                  MYSQL_BIND& bind) {
  auto* optional = static_cast<std::optional<T>*>(value);
  UASSERT(optional);

  // TODO : check this
  if (bind.is_null_value) {
    optional->reset();
  }
}

}  // namespace storages::mysql::impl::bindings

USERVER_NAMESPACE_END
