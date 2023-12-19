#pragma once

#include <string_view>

#include <userver/storages/mysql/impl/io/binder_declarations.hpp>
#include <userver/storages/mysql/impl/io/params_binder_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::io {

class ParamsBinder final : public ParamsBinderBase {
 public:
  explicit ParamsBinder(std::size_t size);
  ~ParamsBinder();

  ParamsBinder(const ParamsBinder& other) = delete;
  ParamsBinder(ParamsBinder&& other) noexcept;

  template <typename Field>
  void Bind(std::size_t pos, const Field& field) {
    storages::mysql::impl::io::BindInput(GetBinds(), pos, field);
  }

  std::size_t GetRowsCount() const final;

  template <typename... Args>
  static ParamsBinder BindParams(const Args&... args) {
    constexpr auto kParamsCount = sizeof...(Args);
    ParamsBinder binder{kParamsCount};

    if constexpr (kParamsCount > 0) {
      size_t ind = 0;
      const auto do_bind = [&binder](std::size_t pos, const auto& arg) {
        // Workaround for "const char*/const char[N]"
        if constexpr (std::is_convertible_v<decltype(arg), const char*>) {
          const auto* const_char_ptr_arg = static_cast<const char*>(arg);
          // std::basic_string_view cannot be constructed from nullptr
          if (const_char_ptr_arg == nullptr) {
            // this is effectively BindNull
            const std::optional<std::string_view> empty_opt_sw{};
            binder.Bind(pos, empty_opt_sw);
          } else {
            const std::string_view sw{arg};
            binder.Bind(pos, sw);
          }
        } else {
          binder.Bind(pos, arg);
        }
      };

      (..., do_bind(ind++, args));
    }

    return binder;
  }
};

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
