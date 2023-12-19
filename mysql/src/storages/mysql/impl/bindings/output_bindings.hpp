#pragma once

#include <vector>

#include <boost/container/small_vector.hpp>

#include <storages/mysql/impl/bindings/native_binds_helper.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::bindings {

class OutputBindings : public NativeBindsHelper {
 public:
  template <typename T>
  using O = std::optional<T>;

  OutputBindings(std::size_t size);

  OutputBindings(const OutputBindings& other) = delete;
  OutputBindings(OutputBindings&& other) noexcept;

  void BeforeFetch(std::size_t pos);
  void AfterFetch(std::size_t pos);

  std::size_t Size() const;
  bool Empty() const;
  MYSQL_BIND* GetBindsArray();
  void WrapBinds(void* binds_array);

  void Bind(std::size_t pos, std::uint8_t& val);
  void Bind(std::size_t pos, O<std::uint8_t>& val);
  void Bind(std::size_t pos, std::int8_t& val);
  void Bind(std::size_t pos, O<std::int8_t>& val);

  void Bind(std::size_t pos, std::uint16_t& val);
  void Bind(std::size_t pos, O<std::uint16_t>& val);
  void Bind(std::size_t pos, std::int16_t& val);
  void Bind(std::size_t pos, O<std::int16_t>& val);

  void Bind(std::size_t pos, std::uint32_t& val);
  void Bind(std::size_t pos, O<std::uint32_t>& val);
  void Bind(std::size_t pos, std::int32_t& val);
  void Bind(std::size_t pos, O<std::int32_t>& val);

  void Bind(std::size_t pos, std::uint64_t& val);
  void Bind(std::size_t pos, O<std::uint64_t>& val);
  void Bind(std::size_t pos, std::int64_t& val);
  void Bind(std::size_t pos, O<std::int64_t>& val);

  void Bind(std::size_t pos, float& val);
  void Bind(std::size_t pos, O<float>& val);
  void Bind(std::size_t pos, double& val);
  void Bind(std::size_t pos, O<double>& val);

  void Bind(std::size_t pos, io::DecimalWrapper& val);
  void Bind(std::size_t pos, O<io::DecimalWrapper>& val);

  void Bind(std::size_t pos, std::string& val);
  void Bind(std::size_t pos, O<std::string>& val);

  void Bind(std::size_t pos, formats::json::Value& val);
  void Bind(std::size_t pos, O<formats::json::Value>& val);

  // These 2 should never be called, so we just leave them unimplemented
  void Bind(std::size_t, std::string_view&) {
    UINVARIANT(false,
               "std::string_view for output binding should be unreachable");
  }
  void Bind(std::size_t, O<std::string_view>&) {
    UINVARIANT(false,
               "std::string_view for output binding should be unreachable");
  }

  void Bind(std::size_t pos, std::chrono::system_clock::time_point& val);
  void Bind(std::size_t pos, O<std::chrono::system_clock::time_point>& val);
  void Bind(std::size_t pos, Date& val);
  void Bind(std::size_t pos, O<Date>& val);
  void Bind(std::size_t pos, DateTime& val);
  void Bind(std::size_t pos, O<DateTime>& val);

  void ValidateAgainstStatement(MYSQL_STMT& statement);

 private:
  MYSQL_BIND& GetBind(std::size_t pos);

  struct FieldIntermediateBuffer {
    io::DecimalWrapper decimal{};  // for un-templated decimals
    MYSQL_TIME time{};             // for dates and the likes
    std::string string{};          // for json and what not
  };

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
  static void PrimitiveOptionalBeforeFetch(void* value, MYSQL_BIND& bind,
                                           FieldIntermediateBuffer&);

  void BindValue(std::size_t pos, enum_field_types type, void* buffer,
                 bool is_unsigned = false);

  // The special problem of binding strings: we don't know the size needed
  // upfront, so after mysql_stmt_fetch we have MYSQL_TRUNCATED and length set,
  // now we have to resize the string accordingly before mysql_stmt_fetch_column
  void BindString(std::size_t pos, std::string& val);
  static void StringBeforeFetch(void* value, MYSQL_BIND& bind,
                                FieldIntermediateBuffer&);

  // The special problem of binding optional strings: same as for strings, but
  // now we have to determine whether it contains a value first, and if it does
  // emplace the optional and resize its string
  void BindOptionalString(std::size_t pos, std::optional<std::string>& val);
  static void OptionalStringBeforeFetch(void* value, MYSQL_BIND& bind,
                                        FieldIntermediateBuffer&);

  // The special problem of binding dates: they are not buffer-wise
  // compatible with C++ types, so we have to store DB date in some
  // storage first, call mysql_stmt_fetch_column and fill the C++ type after
  // that
  void BindTimePoint(std::size_t pos,
                     std::chrono::system_clock::time_point& val);
  static void TimepointAfterFetch(void* value, MYSQL_BIND& bind,
                                  FieldIntermediateBuffer& buffer);

  // The special problem of binding optional dates: same as for dates, but have
  // to emplace the optional first if a value is present
  void BindOptionalTimePoint(
      std::size_t pos,
      std::optional<std::chrono::system_clock::time_point>& val);
  static void OptionalTimePointAfterFetch(void* value, MYSQL_BIND& bind,
                                          FieldIntermediateBuffer& buffer);

  // Date and DateTime are basically the same as TimePoint, they are just easier
  // to restore (they map 1 to 1 to MYSQL_TIME)
  void BindDate(std::size_t pos, Date& val);
  static void DateAfterFetch(void* value, MYSQL_BIND& bind,
                             FieldIntermediateBuffer& buffer);
  void BindOptionalDate(std::size_t pos, std::optional<Date>& val);
  static void OptionalDateAfterFetch(void* value, MYSQL_BIND& bind,
                                     FieldIntermediateBuffer& buffer);

  void BindDateTime(std::size_t pos, DateTime& val);
  static void DateTimeAfterFetch(void* value, MYSQL_BIND& bind,
                                 FieldIntermediateBuffer& buffer);
  void BindOptionalDateTime(std::size_t pos, std::optional<DateTime>& val);
  static void OptionalDateTimeAfterFetch(void* value, MYSQL_BIND& bind,
                                         FieldIntermediateBuffer& buffer);

  // The special problem of binding jsons: basically as with strings, but we
  // can't fetch into json directly, so we use a temporary buffer which we alloc
  // before column fetch and feed into json::Value after column fetch
  void BindJson(std::size_t pos, formats::json::Value& val);
  static void JsonBeforeFetch(void* value, MYSQL_BIND& bind,
                              FieldIntermediateBuffer& buffer);
  static void JsonAfterFetch(void* value, MYSQL_BIND& bind,
                             FieldIntermediateBuffer& buffer);

  // The special problem of binding optional jsons: basically the same as with
  // jsons, but have to determine whether it's a null or not first
  void BindOptionalJson(std::size_t pos,
                        std::optional<formats::json::Value>& val);
  static void OptionalJsonBeforeFetch(void* value, MYSQL_BIND& bind,
                                      FieldIntermediateBuffer& buffer);
  static void OptionalJsonAfterFetch(void* value, MYSQL_BIND& bind,
                                     FieldIntermediateBuffer& buffer);

  // The VERY special problem of binding decimals: decimal::Decimal64 is
  // templated at call site, so we either have to specialize for every possible
  // combination, which is insane, or bring mysql.h in public includes, which is
  // a no-go, or do some dirty type erasure via proxy class and void*.
  // We roll with void*, and what is even more scary is the fact that i'm
  // actually enjoying it and feel very smart about myself.
  // That aside, similar to json: alloc a buffer before
  // fetch, emplace converted data after fetch
  void BindDecimal(std::size_t pos, io::DecimalWrapper& val);
  static void DecimalBeforeFetch(void* value, MYSQL_BIND& bind,
                                 FieldIntermediateBuffer& buffer);
  static void DecimalAfterFetch(void* value, MYSQL_BIND& bind,
                                FieldIntermediateBuffer& buffer);

  // Same problem as with decimal, same solution as with json: if not null -
  // alloc before fetch, emplace with fetched data after fetch
  void BindOptionalDecimal(std::size_t pos, O<io::DecimalWrapper>& val);
  static void OptionalDecimalBeforeFetch(void* value, MYSQL_BIND& bind,
                                         FieldIntermediateBuffer& buffer);
  static void OptionalDecimalAfterFetch(void* value, MYSQL_BIND& bind,
                                        FieldIntermediateBuffer& buffer);

  static void ValidateBind(std::size_t pos, const MYSQL_BIND& bind,
                           const MYSQL_FIELD& field);

  // We use this to perform some kind of type erasure: every instance knows how
  // to restore its type via typed callbacks
  struct BindCallbacks {
    using TypedCallback = void (*)(void* value, MYSQL_BIND& bind,
                                   FieldIntermediateBuffer&);

    void* value{nullptr};
    TypedCallback before_fetch_cb{nullptr};
    TypedCallback after_fetch_cb{nullptr};
  };

  // TODO : merge these 2
  boost::container::small_vector<BindCallbacks, kOnStackBindsCount> callbacks_;
  boost::container::small_vector<FieldIntermediateBuffer, kOnStackBindsCount>
      intermediate_buffers_;
  boost::container::small_vector<MYSQL_BIND, kOnStackBindsCount> owned_binds_;
  // This is either pointing to owned_binds_.data()
  // or to binds array stored inside mysql internals
  MYSQL_BIND* binds_ptr_{nullptr};
};

static_assert(std::is_nothrow_move_constructible_v<OutputBindings>);

template <typename T>
void OutputBindings::BindPrimitiveOptional(std::size_t pos,
                                           std::optional<T>& val,
                                           bool is_unsigned) {
  UASSERT(!val.has_value());

  auto& bind = GetBind(pos);
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
void OutputBindings::PrimitiveOptionalBeforeFetch(void* value, MYSQL_BIND& bind,
                                                  FieldIntermediateBuffer&) {
  auto* optional = static_cast<std::optional<T>*>(value);
  UASSERT(optional);

  if (bind.is_null_value) {
    optional->reset();
  }
}

}  // namespace storages::mysql::impl::bindings

USERVER_NAMESPACE_END
