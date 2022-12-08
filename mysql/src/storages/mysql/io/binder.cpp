#include <userver/storages/mysql/io/binder.hpp>

#include <storages/mysql/impl/mysql_binds_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::io {

#define DEFINE_BINDER(type) \
  void Binder<type>::Bind() { binds_.Bind(pos_, field_); }

DEFINE_BINDER(std::uint8_t)
DEFINE_BINDER(std::int8_t)
DEFINE_BINDER(std::uint32_t)
DEFINE_BINDER(std::int32_t)
DEFINE_BINDER(std::string)
DEFINE_BINDER(std::string_view)

ResultBinder::ResultBinder() : impl_(impl::BindsStorage::BindsType::kResult) {}

ResultBinder::~ResultBinder() = default;

ParamsBinder::ParamsBinder()
    : impl_(impl::BindsStorage::BindsType::kParameters) {}

ParamsBinder::~ParamsBinder() = default;

ParamsBinder::ParamsBinder(ParamsBinder&& other) noexcept = default;

}  // namespace storages::mysql::io

USERVER_NAMESPACE_END
