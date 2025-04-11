#pragma once

#include <userver/storages/sqlite/impl/binder_fwd.hpp>
#include <userver/storages/sqlite/infra/connection_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl::io {

class ParamsBinderBase {
public:
    explicit ParamsBinderBase(const std::string& query, infra::ConnectionPtr& conn);

    ParamsBinderBase(const ParamsBinderBase& other) = delete;
    ParamsBinderBase(ParamsBinderBase&& other) noexcept;

    InputBindingsFwd& GetBinds();

    InputBindingsPimpl GetBindsPtr();

protected:
    ~ParamsBinderBase();

private:
    InputBindingsPimpl binds_impl_;
};

}  // namespace storages::sqlite::impl::io

USERVER_NAMESPACE_END
