#include <userver/storages/sqlite/impl/io/params_binder.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl::io {

ParamsBinder::ParamsBinder(const std::string& query, infra::ConnectionPtr& conn) : ParamsBinderBase{query, conn} {}

ParamsBinder::~ParamsBinder() = default;

ParamsBinder::ParamsBinder(ParamsBinder&& other) noexcept : ParamsBinderBase(std::move(other)) {}

}  // namespace storages::sqlite::impl::io

USERVER_NAMESPACE_END
