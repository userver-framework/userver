#include <userver/storages/mysql/impl/io/params_binder.hpp>

#include <storages/mysql/impl/bindings/input_bindings.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::io {

ParamsBinder::ParamsBinder(std::size_t size) : ParamsBinderBase{size} {}

ParamsBinder::~ParamsBinder() = default;

ParamsBinder::ParamsBinder(ParamsBinder&& other) noexcept
    : ParamsBinderBase(std::move(other)) {}

std::size_t ParamsBinder::GetRowsCount() const { return 1; }

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
