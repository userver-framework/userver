#include <storages/postgres/postgres_config.hpp>

#include <logging/log.hpp>

#include <storages/postgres/component.hpp>
#include <storages/postgres/exceptions.hpp>

namespace storages::postgres {

CommandControl Parse(const formats::json::Value& elem,
                     formats::parse::To<CommandControl>) {
  CommandControl result{components::Postgres::kDefaultCommandControl};
  for (auto it = elem.begin(); it != elem.end(); ++it) {
    const auto& name = it.GetName();
    if (name == "network_timeout_ms") {
      result.execute = std::chrono::milliseconds{it->As<int64_t>()};
      if (result.execute.count() <= 0) {
        throw InvalidConfig{"Invalid network_timeout_ms `" +
                            std::to_string(result.execute.count()) +
                            "` in postgres CommandControl. The timeout must be "
                            "greater than 0."};
      }
    } else if (name == "statement_timeout_ms") {
      result.statement = std::chrono::milliseconds{it->As<int64_t>()};
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

Config::Config(const taxi_config::DocsMap& docs_map)
    : default_command_control{"POSTGRES_DEFAULT_COMMAND_CONTROL", docs_map},
      handlers_command_control{"POSTGRES_HANDLERS_COMMAND_CONTROL", docs_map} {}

}  // namespace storages::postgres
