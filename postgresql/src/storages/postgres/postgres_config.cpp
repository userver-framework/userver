#include <storages/postgres/postgres_config.hpp>

#include <storages/postgres/component.hpp>
#include <storages/postgres/exceptions.hpp>

#include <logging/log.hpp>

namespace storages {
namespace postgres {

CommandControl Parse(const formats::json::Value& elem,
                     formats::parse::To<CommandControl>) {
  CommandControl result{components::Postgres::kDefaultCommandControl};
  for (auto it = elem.begin(); it != elem.end(); ++it) {
    const auto& name = it.GetName();
    if (name == "network_timeout_ms") {
      result.network = TimeoutDuration{it->As<int64_t>()};
      if (result.network.count() <= 0) {
        throw InvalidConfig{"Invalid network_timeout_ms `" +
                            std::to_string(result.network.count()) +
                            "` in postgres CommandControl. The timeout must be "
                            "greater than 0."};
      }
    } else if (name == "statement_timeout_ms") {
      result.statement = TimeoutDuration{it->As<int64_t>()};
      if (result.statement.count() <= 0) {
        throw InvalidConfig{"Invalid statement_timeout_ms `" +
                            std::to_string(result.statement.count()) +
                            "` in postgres CommandControl. The timeout must be "
                            "greater than 0."};
      }
    } else {
      LOG_WARNING() << "Unknown parameter " << name << " in PostgreSQL config";
    }
  }
  return result;
}

}  // namespace postgres
}  // namespace storages
