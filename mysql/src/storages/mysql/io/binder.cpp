#include <userver/storages/mysql/io/binder.hpp>

#include <storages/mysql/impl/bindings/input_bindings.hpp>
#include <storages/mysql/impl/bindings/output_bindings.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::io {

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEFINE_BINDER_T(type)                                         \
  void InputBinder<const type>::Bind() { binds_.Bind(pos_, field_); } \
  void OutputBinder<type>::Bind() { binds_.Bind(pos_, field_); }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEFINE_BINDER_OPTT(type)                        \
  void InputBinder<const std::optional<type>>::Bind() { \
    binds_.Bind(pos_, field_);                          \
  }                                                     \
  void OutputBinder<std::optional<type>>::Bind() { binds_.Bind(pos_, field_); }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEFINE_BINDER(type) \
  DEFINE_BINDER_T(type)     \
  DEFINE_BINDER_OPTT(type)

// numeric types
DEFINE_BINDER(std::uint8_t)
DEFINE_BINDER(std::int8_t)
DEFINE_BINDER(std::uint16_t)
DEFINE_BINDER(std::int16_t)
DEFINE_BINDER(std::uint32_t)
DEFINE_BINDER(std::int32_t)
DEFINE_BINDER(std::uint64_t)
DEFINE_BINDER(std::int64_t)
DEFINE_BINDER(float)
DEFINE_BINDER(double)
// string types
DEFINE_BINDER(std::string)
DEFINE_BINDER(std::string_view)
DEFINE_BINDER(formats::json::Value)
// date types
DEFINE_BINDER(std::chrono::system_clock::time_point)

ResultBinder::ResultBinder(std::size_t size) : impl_{size} {}

ResultBinder::~ResultBinder() = default;

ResultBinder::ResultBinder(ResultBinder&& other) noexcept = default;

impl::bindings::OutputBindings& ResultBinder::GetBinds() { return *impl_; }

ParamsBinder::ParamsBinder(std::size_t size) : impl_{size} {}

ParamsBinder::~ParamsBinder() = default;

ParamsBinder::ParamsBinder(ParamsBinder&& other) noexcept = default;

impl::bindings::InputBindings& ParamsBinder::GetBinds() { return *impl_; }

}  // namespace storages::mysql::io

USERVER_NAMESPACE_END
