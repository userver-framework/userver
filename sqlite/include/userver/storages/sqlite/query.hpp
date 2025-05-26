#pragma once

/// @file userver/storages/sqlite/query.hpp

#include <optional>
#include <string>

#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

// TODO: Can use Query from #include <userver/storages/query.hpp>?

/// @brief Query class, which driver executes.
class Query {
public:
    /// @brief Strong typedef for query name, one can use named queries to get
    /// better logging experience
    using Name = utils::StrongTypedef<struct NameTag, std::string>;

    /// @brief Query constructor
    Query(const char* statement, std::optional<Name> = std::nullopt);

    /// @brief Query constructor
    Query(std::string statement, std::optional<Name> = std::nullopt);

    /// @brief Get query statement
    const std::string& GetStatement() const;

    /// @brief Get query name
    const std::optional<Name>& GetName() const;

private:
    std::string statement_;
    std::optional<Name> name_;
};

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
