#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <boost/pfr/core.hpp>

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace impl {
class BindsStorage;

using BindsPimpl = utils::FastPimpl<BindsStorage, 56, 8>;
}  // namespace impl

namespace io {

template <typename Field>
class BinderBase {
 public:
  BinderBase(impl::BindsStorage& binds, std::size_t pos, Field& field)
      : binds_{binds}, pos_{pos}, field_{field} {}

  void operator()() { Bind(); }

 protected:
  virtual void Bind() = 0;

  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  impl::BindsStorage& binds_;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::size_t pos_;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  Field& field_;
};

template <typename T>
class Binder final {
  Binder(impl::BindsStorage&, std::size_t, T&) {
    static_assert(!sizeof(T), "Binding for the type is not implemented");
  }
};

#define DECLARE_BINDER(type)                           \
  template <>                                          \
  class Binder<type> final : public BinderBase<type> { \
    using BinderBase::BinderBase;                      \
    void Bind() final;                                 \
  };

DECLARE_BINDER(std::uint8_t)
DECLARE_BINDER(std::int8_t)
DECLARE_BINDER(std::int32_t)
DECLARE_BINDER(std::uint32_t)
DECLARE_BINDER(std::string)
DECLARE_BINDER(std::string_view)

template <typename T>
auto GetBinder(impl::BindsStorage& binds, std::size_t pos, T& field) {
  return Binder<T>{binds, pos, field};
}

class ParamsBinderBase {
 public:
  virtual impl::BindsStorage& GetBinds() = 0;
};

class ParamsBinder final : public ParamsBinderBase {
 public:
  ParamsBinder();
  ~ParamsBinder();

  ParamsBinder(const ParamsBinder& other) = delete;
  ParamsBinder(ParamsBinder&& other) noexcept;

  impl::BindsStorage& GetBinds() final;

  template <typename Field>
  void Bind(std::size_t pos, Field& field) {
    GetBinder(*impl_, pos, field)();
  }

  template <typename... Args>
  static ParamsBinder BindParams(Args&... args) {
    ParamsBinder binder{};
    size_t ind = 0;

    const auto do_bind = [&binder](std::size_t pos, auto& arg) {
      if constexpr (std::is_convertible_v<decltype(arg), std::string_view>) {
        std::string_view sw{arg};
        binder.Bind(pos, sw);
      } else {
        binder.Bind(pos, arg);
      }
    };

    (..., do_bind(ind++, args));

    return binder;
  }

 private:
  impl::BindsPimpl impl_;
};

class FieldBinder final {
 public:
  explicit FieldBinder(impl::BindsStorage& binds) : binds_{binds} {}

  template <typename Field, size_t Index>
  void operator()(Field& field, std::integral_constant<std::size_t, Index> i) {
    GetBinder(binds_, i, field)();
  }

 private:
  impl::BindsStorage& binds_;
};

class ResultBinder final {
 public:
  ResultBinder();
  ~ResultBinder();

  template <typename T>
  impl::BindsStorage& BindTo(T& row) {
    boost::pfr::for_each_field(row, FieldBinder{*impl_});

    return *impl_;
  }

 private:
  impl::BindsPimpl impl_;
};

}  // namespace io

}  // namespace storages::mysql

USERVER_NAMESPACE_END
