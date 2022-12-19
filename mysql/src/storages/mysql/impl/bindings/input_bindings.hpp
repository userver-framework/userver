#pragma once

#include <vector>

#include <boost/container/small_vector.hpp>

#include <storages/mysql/impl/bindings/binds_storage_interface.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::bindings {

class InputBindings final : public BindsStorageInterface<true> {
 public:
  InputBindings(std::size_t size);

  InputBindings(const InputBindings& other) = delete;
  InputBindings(InputBindings&& other) noexcept;

  using ParamsCallback = void (*)(void*, void*, std::size_t);

  void SetParamsCallback(ParamsCallback params_cb);
  ParamsCallback GetParamsCallback() const;
  void SetUserData(void* user_data);
  void* GetUserData() const;

  std::size_t Size() const final;
  bool Empty() const final;
  MYSQL_BIND* GetBindsArray() final;
  void WrapBinds(void* binds_array) final;

  void Bind(std::size_t pos, C<std::uint8_t>& val) final;
  void Bind(std::size_t pos, C<O<std::uint8_t>>& val) final;
  void Bind(std::size_t pos, C<std::int8_t>& val) final;
  void Bind(std::size_t pos, C<O<std::int8_t>>& val) final;

  void Bind(std::size_t pos, C<std::uint16_t>& val) final;
  void Bind(std::size_t pos, C<O<std::uint16_t>>& val) final;
  void Bind(std::size_t pos, C<std::int16_t>& val) final;
  void Bind(std::size_t pos, C<O<std::int16_t>>& val) final;

  void Bind(std::size_t pos, C<std::uint32_t>& val) final;
  void Bind(std::size_t pos, C<O<std::uint32_t>>& val) final;
  void Bind(std::size_t pos, C<std::int32_t>& val) final;
  void Bind(std::size_t pos, C<O<std::int32_t>>& val) final;

  void Bind(std::size_t pos, C<std::uint64_t>& val) final;
  void Bind(std::size_t pos, C<O<std::uint64_t>>& val) final;
  void Bind(std::size_t pos, C<std::int64_t>& val) final;
  void Bind(std::size_t pos, C<O<std::int64_t>>& val) final;

  void Bind(std::size_t pos, C<float>& val) final;
  void Bind(std::size_t pos, C<O<float>>& val) final;
  void Bind(std::size_t pos, C<double>& val) final;
  void Bind(std::size_t pos, C<O<double>>& val) final;

  void Bind(std::size_t pos, C<io::DecimalWrapper>& val) final;
  void Bind(std::size_t pos, C<O<io::DecimalWrapper>>& val) final;

  void Bind(std::size_t pos, C<std::string>& val) final;
  void Bind(std::size_t pos, C<O<std::string>>& val) final;
  void Bind(std::size_t pos, C<std::string_view>& val) final;
  void Bind(std::size_t pos, C<O<std::string_view>>& val) final;

  void Bind(std::size_t pos, C<formats::json::Value>& val) final;
  void Bind(std::size_t pos, C<O<formats::json::Value>>& val) final;

  void Bind(std::size_t pos,
            C<std::chrono::system_clock::time_point>& val) final;
  void Bind(std::size_t pos,
            C<O<std::chrono::system_clock::time_point>>& val) final;

  void ValidateAgainstStatement(MYSQL_STMT& statement) final;

 private:
  MYSQL_BIND& GetBind(std::size_t pos);

  template <typename COT>
  void BindOptional(std::size_t pos, COT& val);

  void BindNull(std::size_t pos);

  template <typename CT>
  void BindValue(std::size_t pos, enum_field_types type, CT& val,
                 std::size_t size, bool is_unsigned = false);

  void BindDate(std::size_t pos, C<std::chrono::system_clock::time_point>& val);

  void BindStringView(std::size_t pos, C<std::string_view>& val);

  void BindJson(std::size_t pos, C<formats::json::Value>& val);

  void BindDecimal(std::size_t pos, C<io::DecimalWrapper>& val);

  struct FieldIntermediateBuffer final {
    MYSQL_TIME time{};     // for dates and the likes
    std::string string{};  // for json and what not
  };
  boost::container::small_vector<MYSQL_BIND, kOnStackBindsCount> owned_binds_;
  boost::container::small_vector<FieldIntermediateBuffer, kOnStackBindsCount>
      intermediate_buffers_;

  ParamsCallback params_cb_{nullptr};
  void* user_data_{nullptr};

  // This is either pointing to owned_binds_.data()
  // or to binds array stored inside mysql internals (happens with batch insert)
  MYSQL_BIND* binds_ptr_{nullptr};
};

static_assert(std::is_nothrow_move_constructible_v<InputBindings>);

template <typename COT>
void InputBindings::BindOptional(std::size_t pos, COT& val) {
  if (val.has_value()) {
    Bind(pos, *val);
  } else {
    BindNull(pos);
  }
}

template <typename CT>
void InputBindings::BindValue(std::size_t pos, enum_field_types type, CT& val,
                              std::size_t size, bool is_unsigned) {
  UASSERT(pos < Size());

  const void* buffer = &val;

  auto& bind = GetBind(pos);
  bind.buffer_type = type;
  // Underlying mysql native library uses mutable pointers for bind buffers, so
  // we have to cast const away sooner or later. This is fine, since buffer are
  // only read from for input parameters.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  bind.buffer = const_cast<void*>(buffer);
  bind.buffer_length = size;
  bind.is_unsigned = is_unsigned;
}

}  // namespace storages::mysql::impl::bindings

USERVER_NAMESPACE_END
