#include <userver/storages/mysql/impl/io/binder_declarations.hpp>

#include <storages/mysql/impl/bindings/input_bindings.hpp>
#include <storages/mysql/impl/bindings/output_bindings.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::io {

// Tidy gets upset with <type> and insists on parentheses around 'type',
// which is invalid C++.
// Tidy also heavily dislikes macro usage, but these are just a bunch of codegen
// macros, so idc.
// NOLINTBEGIN
#define DEFINE_INPUT_BINDER(type)                               \
  void InputBinder<type>::Bind() { binds_.Bind(pos_, field_); } \
  void InputBinder<std::optional<type>>::Bind() { binds_.Bind(pos_, field_); }

#define DEFINE_OUTPUT_BINDER(type)                               \
  void OutputBinder<type>::Bind() { binds_.Bind(pos_, field_); } \
  void OutputBinder<std::optional<type>>::Bind() { binds_.Bind(pos_, field_); }

#define DEFINE_BINDER(type) \
  DEFINE_INPUT_BINDER(type) \
  DEFINE_OUTPUT_BINDER(type)
// NOLINTEND

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
DEFINE_BINDER(DecimalWrapper)
DEFINE_BINDER(std::string)
DEFINE_BINDER(std::string_view)
DEFINE_BINDER(formats::json::Value)
// date types
DEFINE_BINDER(std::chrono::system_clock::time_point)

#undef DEFINE_INPUT_BINDER
#undef DEFINE_OUTPUT_BINDER
#undef DEFINE_BINDER

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
