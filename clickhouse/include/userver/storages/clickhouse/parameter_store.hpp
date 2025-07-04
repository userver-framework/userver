#pragma once

#include <fmt/args.h>
#include <fmt/format.h>

#include <userver/storages/clickhouse/io/impl/escape.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse {

/// @ingroup userver_containers
///
/// @brief Class for dynamic ClickHouse parameter list construction.
///
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

    /// Returns whether the parameter list is empty.
    bool IsEmpty() const;

    /// Returns current size of the list.
    size_t Size() const;

    /// @brief Get serialized parameters for substitution in a query.
    const fmt::dynamic_format_arg_store<fmt::format_context>& GetParameters() const;

private:
    fmt::dynamic_format_arg_store<fmt::format_context> parameters_{};
};

}  // namespace storages::clickhouse

USERVER_NAMESPACE_END
