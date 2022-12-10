#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include <storages/mysql/impl/mariadb_include.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

class BindsStorage final {
 public:
  enum class BindsType { kParameters, kResult };

  explicit BindsStorage(BindsType binds_type);
  ~BindsStorage();

  class At final {
   public:
    At(BindsStorage& storage, std::size_t pos);

    void Bind(std::uint8_t& val);
    void Bind(std::int8_t& val);

    void Bind(std::uint16_t& val);
    void Bind(std::int16_t& val);

    void Bind(std::uint32_t& val);
    void Bind(std::int32_t& val);

    void Bind(std::uint64_t& val);
    void Bind(std::int64_t& val);

    void Bind(float& val);
    void Bind(double& val);

    void Bind(std::string& val);
    void Bind(std::string_view& val);

    void Bind(std::chrono::system_clock::time_point& val);

   private:
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

  struct OutputString final {
    std::string* output{};
    std::size_t output_length{0};
  };

  struct DateWrapper final {
    std::chrono::system_clock::time_point* output{nullptr};
    MYSQL_TIME native_time{};
  };

  BindsType binds_type_;

  std::vector<OutputString> output_strings_;
  std::vector<DateWrapper> wrapped_dates_;
  std::vector<MYSQL_BIND> binds_;

  void* user_data_{nullptr};
  void (*param_cb_)(void*, void*, std::size_t){nullptr};
};

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
