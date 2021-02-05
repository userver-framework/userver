#pragma once

#include <optional>
#include <string>

#include <utils/strong_typedef.hpp>

namespace storages::postgres {

class Query {
 public:
  using Name = ::utils::StrongTypedef<struct NameTag, std::string>;

  Query() = default;

  Query(const char* statement, std::optional<Name> name = std::nullopt);

  Query(std::string statement, std::optional<Name> name = std::nullopt);

  const std::optional<Name>& GetName() const;

  const std::string& Statement() const;

 private:
  std::string statement_{};
  std::optional<Name> name_{};
};

}  // namespace storages::postgres
