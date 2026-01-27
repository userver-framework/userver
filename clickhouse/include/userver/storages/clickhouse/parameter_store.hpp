#pragma once

/// @file userver/storages/clickhouse/parameter_store.hpp
/// @brief @copybrief storages::clickhouse::ParameterStore

#include <string_view>

#include <fmt/args.h>
#include <fmt/format.h>

#include <userver/storages/clickhouse/io/impl/escape.hpp>
#include <userver/storages/query.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse {

/// @ingroup userver_containers
///
/// @brief Class for dynamic ClickHouse parameter list construction.
///
/// @snippet storages/tests/parameter_store_chtest.cpp  basic usage
class ParameterStore {
public:
    ParameterStore() = default;
    ParameterStore(const ParameterStore&) = delete;
    ParameterStore(ParameterStore&&) = default;
    ParameterStore& operator=(const ParameterStore&) = delete;
    ParameterStore& operator=(ParameterStore&&) = default;

    /// @brief Adds a parameter to the end of the parameter list.
    /// @note Currently only built-in/system types are supported.
    template <typename T>
    ParameterStore& PushBack(const T& param) {
        parameters_.push_back(io::impl::Escape(param));
        return *this;
    }

    /// @cond
    // For internal use only
    Query MakeQueryWithArgs(const Query& query) const {
        // we should throw on params count mismatch
        // TODO : https://st.yandex-team.ru/TAXICOMMON-5066
        return Query{fmt::vformat(std::string_view{query.GetStatementView()}, parameters_), query.GetOptionalName()};
    }
    /// @endcond

private:
    fmt::dynamic_format_arg_store<fmt::format_context> parameters_{};
};

}  // namespace storages::clickhouse

USERVER_NAMESPACE_END
