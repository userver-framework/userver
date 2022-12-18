#pragma once

#include <optional>
#include <string>

#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

class Query final {
 public:
  using Name = utils::StrongTypedef<struct NameTag, std::string>;

  Query(const char* statement, std::optional<Name> name = std::nullopt);

  Query(std::string statement, std::optional<Name> name = std::nullopt);

  const std::string& GetStatement() const;

  const std::optional<Name> GetName() const;

 private:
  std::string statement_;
  std::optional<Name> name_;
};

}  // namespace storages::mysql

USERVER_NAMESPACE_END
