#pragma once

/// @file userver/storages/sqlite/query.hpp

#include <optional>
#include <string>

#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

// TODO: Can use Query from #include <userver/storages/query.hpp>?

class Query {
public:
    using Name = utils::StrongTypedef<struct NameTag, std::string>;
    Query(const char* statement, std::optional<Name> = std::nullopt);
    Query(std::string statement, std::optional<Name> = std::nullopt);

    const std::string& GetStatement() const;
    const std::optional<Name>& GetName() const;

private:
    std::string statement_;
    std::optional<Name> name_;
};

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
