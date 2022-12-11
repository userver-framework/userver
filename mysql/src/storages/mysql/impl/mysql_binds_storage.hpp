#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <userver/utils/assert.hpp>

#include <storages/mysql/impl/mariadb_include.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

template <typename T>
constexpr enum_field_types GetNativeType() {
  if constexpr (std::is_same_v<std::uint8_t, T> ||
                std::is_same_v<std::int8_t, T>) {
    return MYSQL_TYPE_TINY;
  } else if constexpr (std::is_same_v<std::uint16_t, T> ||
                       std::is_same_v<std::int16_t, T>) {
    return MYSQL_TYPE_SHORT;
  } else if constexpr (std::is_same_v<std::uint32_t, T> ||
                       std::is_same_v<std::int32_t, T>) {
    return MYSQL_TYPE_LONG;
  } else if constexpr (std::is_same_v<std::uint64_t, T> ||
                       std::is_same_v<std::int64_t, T>) {
    return MYSQL_TYPE_LONGLONG;
  } else if constexpr (std::is_same_v<float, T>) {
    return MYSQL_TYPE_FLOAT;
  } else if constexpr (std::is_same_v<double, T>) {
    return MYSQL_TYPE_DOUBLE;
  } else if constexpr (std::is_same_v<std::string, T>) {
    return MYSQL_TYPE_STRING;
  } else if constexpr (std::is_same_v<std::chrono::system_clock::time_point,
                                      T>) {
    return MYSQL_TYPE_DATETIME;
  } else {
    static_assert(!sizeof(T), "This shouldn't fire, fix me");
  }
}

class BindsStorage final {
 public:
  enum class BindsType { kParameters, kResult };

  explicit BindsStorage(BindsType binds_type);
  ~BindsStorage();

  class At final {
   public:
    At(BindsStorage& storage, std::size_t pos);

    template <typename T>
    using O = std::optional<T>;

    void Bind(std::uint8_t& val);
    void Bind(O<std::uint8_t>& val);
    void Bind(std::int8_t& val);
    void Bind(O<std::int8_t>& val);

    void Bind(std::uint16_t& val);
    void Bind(O<std::uint16_t>& val);
    void Bind(std::int16_t& val);
    void Bind(O<std::int16_t>& val);

    void Bind(std::uint32_t& val);
    void Bind(O<std::uint32_t>& val);
    void Bind(std::int32_t& val);
    void Bind(O<std::int32_t>& val);

    void Bind(std::uint64_t& val);
    void Bind(O<std::uint64_t>& val);
    void Bind(std::int64_t& val);
    void Bind(O<std::int64_t>& val);

    void Bind(float& val);
    void Bind(O<float>& val);
    void Bind(double& val);
    void Bind(O<double>& val);

    void Bind(std::string& val);
    void Bind(O<std::string>& val);
    void Bind(std::string_view& val);

    void Bind(std::chrono::system_clock::time_point& val);
    void Bind(O<std::chrono::system_clock::time_point>& val);

   private:
    template <typename T>
    void BindOptional(std::optional<T>& val) {
      if (val.has_value()) {
        Bind(*val);
      } else {
        if (storage_.binds_type_ == BindsType::kParameters) {
          DoBindNull();
        } else {
          storage_.DoBindOutputNull(pos_, val);
        }
      }
    }

    void DoBindNull();

    BindsStorage& storage_;
    std::size_t pos_;
  };
  At GetAt(std::size_t pos);

  // there functions are called when parsing statement execution result
  void UpdateBeforeFetch(std::size_t pos);
  void UpdateAfterFetch(std::size_t pos);

  MYSQL_BIND* GetBindsArray();

  std::size_t Size() const;
  bool Empty() const;

  void SetBindCallbacks(void* user_data,
                        void (*param_cb)(void*, void*, std::size_t));

  void* GetUserData() const;
  void* GetParamCb() const;

 private:
  void ResizeIfNecessary(std::size_t pos);

  void DoBindOutputString(std::size_t pos, std::string& val);

  using MariaDBTimepointResolution = std::chrono::microseconds;
  void DoBindDate(std::size_t pos, std::chrono::system_clock::time_point& val);
  void DoBindOutputDate(std::size_t pos,
                        std::chrono::system_clock::time_point& val);

  void ResizeOutputString(std::size_t pos);
  void FillOutputDate(std::size_t pos);

  void DoBind(std::size_t pos, enum_field_types type, void* buffer,
              std::size_t length, bool is_unsigned = false);

  template <typename T>
  void DoBindOutputNull(std::size_t pos, std::optional<T>& val) {
    ResizeIfNecessary(pos);

    auto& opt = output_optionals_[pos];
    auto& bind = binds_[pos];

    opt.optional = &val;
    OutputOptionalCallbacksTraits<T>::InitCallbacks(opt);

    bind.buffer_type = MYSQL_TYPE_NULL;
    bind.buffer = nullptr;
    bind.buffer_length = 0;
    bind.is_null = &opt.is_null;

    // if constexpr (std::is_same_v<std::string, T>) {
    bind.length = &opt.length;
    //}

    if constexpr (std::is_same_v<std::chrono::system_clock::time_point, T>) {
      bind.buffer = &wrapped_dates_[pos].native_time;
      bind.buffer_length = sizeof(MYSQL_TIME);
      bind.length = nullptr;
    }
  }

  struct OutputString final {
    std::string* output{};
    std::size_t output_length{0};
  };

  struct DateWrapper final {
    std::chrono::system_clock::time_point* output{nullptr};
    MYSQL_TIME native_time{};

    void UpdateOutput(void* ptr) {
      output = static_cast<std::chrono::system_clock::time_point*>(ptr);
    }
  };

  struct OutputOptional final {
    using EmplaceCb = void (*)(OutputOptional*, void*);
    using GetDataPtrFn = void* (*)(void*);
    using FixBindCb = void (*)(OutputOptional*, MYSQL_BIND*);
    using ResetCb = void (*)(void*);

    void* optional{nullptr};
    std::size_t length{0};
    my_bool is_null{1};

    EmplaceCb emplace_cb{nullptr};
    GetDataPtrFn get_data_ptr{nullptr};
    FixBindCb fix_bind_cb{nullptr};
    ResetCb reset_cb{nullptr};
  };
  template <typename T>
  struct OutputOptionalCallbacksTraits final {
    using O = std::optional<T>;

    static void Emplace(OutputOptional* self, void* opt) {
      auto* optional = static_cast<O*>(opt);
      UASSERT(optional);
      optional->emplace();

      if constexpr (std::is_same_v<std::string, T>) {
        (*optional)->resize(self->length);
      }
    }

    static void* GetDataPtr(void* opt) {
      auto* optional = static_cast<O*>(opt);
      UASSERT(optional->has_value());

      if constexpr (std::is_same_v<std::string, T>) {
        return (*optional)->data();
      } else {
        return &(**optional);
      }
    }

    static void FixBind(OutputOptional* self, MYSQL_BIND* bind) {
      bind->buffer_type = GetNativeType<T>();
      if constexpr (std::is_same_v<std::string, T>) {
        bind->buffer_length = self->length;
      }
    }

    static void Reset(void* opt) {
      auto* optional = static_cast<O*>(opt);
      UASSERT(optional);
      optional->reset();
    }

    static void InitCallbacks(OutputOptional& output_optional) {
      output_optional.emplace_cb = &Emplace;
      output_optional.get_data_ptr = &GetDataPtr;
      output_optional.fix_bind_cb = &FixBind;
      output_optional.reset_cb = &Reset;
    }
  };

  BindsType binds_type_;

  std::vector<OutputOptional> output_optionals_;
  std::vector<OutputString> output_strings_;
  std::vector<DateWrapper> wrapped_dates_;
  std::vector<MYSQL_BIND> binds_;

  void* user_data_{nullptr};
  void (*param_cb_)(void*, void*, std::size_t){nullptr};
};

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
