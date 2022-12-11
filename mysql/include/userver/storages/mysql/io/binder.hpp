#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <boost/pfr/core.hpp>

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace impl {
class BindsStorage;

using BindsPimpl = utils::FastPimpl<BindsStorage, 120, 8>;
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
 public:
  Binder(impl::BindsStorage&, std::size_t, T&) {
    using SteadyClock = std::chrono::steady_clock;
    if constexpr (std::is_same_v<SteadyClock::time_point, T> ||
                  std::is_same_v<std::optional<SteadyClock::time_point>, T>) {
      static_assert(!sizeof(T),
                    "Don't store steady_clock times in the database, use "
                    "system_clock instead");
    }
    static_assert(!sizeof(T), "Binding for the type is not implemented");
  }
};

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECLARE_BINDER_T(type)                         \
  template <>                                          \
  class Binder<type> final : public BinderBase<type> { \
    using BinderBase::BinderBase;                      \
    void Bind() final;                                 \
  };

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECLARE_BINDER_OPTT(type)                \
  template <>                                    \
  class Binder<std::optional<type>> final        \
      : public BinderBase<std::optional<type>> { \
    using BinderBase::BinderBase;                \
    void Bind() final;                           \
  };

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECLARE_BINDER(type) \
  DECLARE_BINDER_T(type)     \
  DECLARE_BINDER_OPTT(type)

// numeric types
DECLARE_BINDER(std::uint8_t)
DECLARE_BINDER(std::int8_t)
DECLARE_BINDER(std::uint16_t)
DECLARE_BINDER(std::int16_t)
DECLARE_BINDER(std::int32_t)
DECLARE_BINDER(std::uint32_t)
DECLARE_BINDER(std::uint64_t)
DECLARE_BINDER(std::int64_t)
DECLARE_BINDER(float);
DECLARE_BINDER(double);
// string types
DECLARE_BINDER(std::string)
DECLARE_BINDER_T(std::string_view)
// date types
DECLARE_BINDER(std::chrono::system_clock::time_point);

template <typename T>
auto GetBinder(impl::BindsStorage& binds, std::size_t pos, T& field) {
  using Mutable = std::remove_const_t<T>;

  // Underlying mysql native library uses mutable pointers for bind buffers, so
  // we have to cast const away sooner or later. This is fine, since buffer are
  // only read from for input parameters, and for output ones we never have
  // const here.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  return Binder<Mutable>{binds, pos, const_cast<Mutable&>(field)};
}

class ParamsBinderBase {
 public:
  virtual impl::BindsStorage& GetBinds() = 0;

  virtual std::size_t GetRowsCount() const { return 0; }
};

class ParamsBinder final : public ParamsBinderBase {
 public:
  ParamsBinder();
  ~ParamsBinder();

  ParamsBinder(const ParamsBinder& other) = delete;
  ParamsBinder(ParamsBinder&& other) noexcept;

  impl::BindsStorage& GetBinds() final;

  template <typename Field>
  void Bind(std::size_t pos, const Field& field) {
    GetBinder(GetBinds(), pos, field)();
  }

  std::size_t GetRowsCount() const final { return 1; }

  template <typename... Args>
  static ParamsBinder BindParams(const Args&... args) {
    ParamsBinder binder{};
    size_t ind = 0;

    const auto do_bind = [&binder](std::size_t pos, const auto& arg) {
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

class ResultBinder final {
 public:
  ResultBinder();
  ~ResultBinder();

  ResultBinder(const ResultBinder& other) = delete;
  ResultBinder(ResultBinder&& other) noexcept;

  template <typename T>
  impl::BindsStorage& BindTo(T& row) {
    boost::pfr::for_each_field(row, FieldBinder{*impl_});

    return GetBinds();
  }

  impl::BindsStorage& GetBinds();

 private:
  class FieldBinder final {
   public:
    explicit FieldBinder(impl::BindsStorage& binds) : binds_{binds} {}

    template <typename Field, size_t Index>
    void operator()(Field& field,
                    std::integral_constant<std::size_t, Index> i) {
      GetBinder(binds_, i, field)();
    }

   private:
    impl::BindsStorage& binds_;
  };

  impl::BindsPimpl impl_;
};

}  // namespace io

}  // namespace storages::mysql

USERVER_NAMESPACE_END
