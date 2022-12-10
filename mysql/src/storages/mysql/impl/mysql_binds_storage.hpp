#pragma once

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

  void Bind(std::size_t pos, std::int8_t& val);
  void Bind(std::size_t pos, std::uint8_t& val);

  void Bind(std::size_t pos, std::int32_t& val);
  void Bind(std::size_t pos, std::uint32_t& val);

  void Bind(std::size_t pos, std::string& val);
  void Bind(std::size_t pos, std::string_view& val);

  void ResizeOutputString(std::size_t pos);

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

  void DoBind(std::size_t pos, enum_field_types type, void* buffer,
              std::size_t length, bool is_unsigned = false);

  struct OutputString final {
    std::string* output{};
    std::size_t output_length{0};
  };

  BindsType binds_type_;

  std::vector<OutputString> output_strings_;
  std::vector<MYSQL_BIND> binds_;

  void* user_data_{nullptr};
  void (*param_cb_)(void*, void*, std::size_t){nullptr};
};

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
