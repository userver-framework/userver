#pragma once

#include <boost/pfr/core.hpp>

#include <userver/storages/sqlite/impl/io/params_binder.hpp>
#include <userver/storages/sqlite/infra/connection_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

class BindHelper final {
public:
    template <typename... Args>
    static io::ParamsBinder
    UpdateParamsBindings(const std::string& query, infra::ConnectionPtr& conn, const Args&... args) {
        return io::ParamsBinder::BindParams(query, conn, args...);
    }

    template <typename T>
    static io::ParamsBinder
    UpdateRowAsParamsBindings(const std::string& query, infra::ConnectionPtr& conn, const T& row) {
        static_assert(
            std::is_aggregate_v<T> || boost::pfr::tuple_size_v<T> > 0, "T must be an aggregate type or tuple-like type"
        );
        if constexpr (std::is_aggregate_v<T>) {
            return std::apply(
                [&](const auto&... args) { return impl::io::ParamsBinder::BindParams(query, conn, args...); },
                boost::pfr::structure_to_tuple(row)
            );
        } else {
            return std::apply(
                [&](const auto& query, auto& conn, const auto&... args) {
                    return impl::io::ParamsBinder::BindParams(query, conn, args...);
                },
                row
            );
        }
    }
};

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
