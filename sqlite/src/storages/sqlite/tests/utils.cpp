#include <userver/storages/sqlite/tests/utils.hpp>

#include <cstdlib>
#include <optional>
#include <string>

#include <fmt/format.h>
#include <userver/components/component_config.hpp>
#include <userver/engine/subprocess/process_starter.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/formats/yaml.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/utils/from_string.hpp>
#include <userver/utils/uuid4.hpp>

#include "userver/storages/sqlite/options.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

// namespace {

// std::string GenerateTableName() {
//   auto uuid = utils::generators::GenerateUuid();

//   std::string name{"tmp_"};
//   name.reserve(4 + uuid.size());
//   for (const auto c : uuid) {
//     if (c != '-') {
//       name.push_back(c);
//     }
//   }

//   return name;
// }

// ConnectionPtr CreateConnection(
//     [[maybe_unused]] bool in_memory = true,
//     [[maybe_unused]] std::optional<std::string> path_to_db = std::nullopt) {
//   //   CreateTestDatabase();
//   SQLiteSettings settings;

//   //   const components::ComponentConfig config{
//   //       yaml_config::YamlConfig{formats::yaml::FromString(R"(
//   //     path_to_db: ::
//   //   )"),
//   return std::make_shared<Connection>();
// }

// }  // namespace

// ConnectionWrapper::ConnectionWrapper() : connection_{CreateConnection()} {}

// ConnectionWrapper::~ConnectionWrapper() = default;

// Connection& ConnectionWrapper::operator*() const { return *connection_; }

// Connection* ConnectionWrapper::operator->() const { return connection_.get();
// }

// TmpTable::TmpTable(std::string_view definition)
//     : owned_connection_{std::in_place},
//       connection_{*owned_connection_},
//       table_name_{GenerateTableName()} {
//   CreateTable(definition);
// }

// TmpTable::TmpTable(ConnectionWrapper& connection, std::string_view
// definition)
//     : connection_{connection}, table_name_{GenerateTableName()} {
//   CreateTable(definition);
// }

// TmpTable::~TmpTable() = default;

// ConnectionWrapper& TmpTable::GetConnection() const { return connection_; }

// Transaction TmpTable::Begin(const std::string& name,
//                             TransactionOptions options) {
//   return connection_->Begin(name, options);
// }

// void TmpTable::CreateTable(std::string_view definition) {
//   const auto create_table_query =
//       fmt::format(kCreateTableQueryTemplate, table_name_, definition);

//   connection_->Execute(create_table_query);
// }

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
