#include <userver/storages/mysql/io/binder.hpp>

#include <storages/mysql/impl/mysql_binds_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::io {

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEFINE_BINDER(type) \
  void Binder<type>::Bind() { binds_.GetAt(pos_).Bind(field_); }

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
// date types
DEFINE_BINDER(std::chrono::system_clock::time_point)

ResultBinder::ResultBinder() : impl_(impl::BindsStorage::BindsType::kResult) {}

ResultBinder::~ResultBinder() = default;

ResultBinder::ResultBinder(ResultBinder&& other) noexcept = default;

impl::BindsStorage& ResultBinder::GetBinds() { return *impl_; }

ParamsBinder::ParamsBinder()
    : impl_(impl::BindsStorage::BindsType::kParameters) {}

ParamsBinder::~ParamsBinder() = default;

ParamsBinder::ParamsBinder(ParamsBinder&& other) noexcept = default;

impl::BindsStorage& ParamsBinder::GetBinds() { return *impl_; }

}  // namespace storages::mysql::io

USERVER_NAMESPACE_END
