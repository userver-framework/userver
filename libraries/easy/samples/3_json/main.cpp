#include <userver/utest/using_namespace_userver.hpp>  // Note: this is for the purposes of samples only

#include <userver/easy.hpp>
#include "schemas/key_value.hpp"

constexpr std::string_view kSchema = R"~(
CREATE TABLE IF NOT EXISTS key_value_table (
  key integer PRIMARY KEY,
  value VARCHAR
)
)~";

int main(int argc, char* argv[]) {
    easy::HttpWith<easy::PgDep>(argc, argv)
        .DbSchema(kSchema)
        .Get(
            "/kv",
            [](formats::json::Value request_json, const easy::PgDep& dep) {
                // Use generated parser for As()
                auto key = request_json.As<schemas::KeyRequest>().key;

                auto res = dep.pg().Execute(
                    storages::postgres::ClusterHostType::kSlave, "SELECT value FROM key_value_table WHERE key=$1", key
                );

                const schemas::KeyValue response{key, res[0][0].As<std::string>()};
                return formats::json::ValueBuilder{response}.ExtractValue();
            }
        )
        .Post("/kv", [](formats::json::Value request_json, easy::PgDep dep) {
            // Use generated parser for As()
            auto key_value = request_json.As<schemas::KeyValue>();

            dep.pg().Execute(
                storages::postgres::ClusterHostType::kMaster,
                "INSERT INTO key_value_table(key, value) VALUES($1, $2) ON CONFLICT (key) DO UPDATE SET value = $2",
                key_value.key,
                key_value.value
            );

            return formats::json::Value{};
        });
}
