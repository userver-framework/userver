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
            [](schemas::KeyRequest&& request, const easy::PgDep& dep) {
                auto res = dep.pg().Execute(
                    storages::postgres::ClusterHostType::kSlave,
                    "SELECT value FROM key_value_table WHERE key=$1",
                    request.key
                );

                return schemas::KeyValue{std::move(request.key), res[0][0].As<std::string>()};
            }
        )
        .Post("/kv", [](schemas::KeyValue key_value, easy::PgDep dep) {
            if (key_value.key == 42) {
                throw server::handlers::ClientError(
                    server::handlers::ExternalBody{"We do not accept key 42"},  // goes to HTTP response
                    server::handlers::InternalMessage{"User sent a 42 key"}     // goes to logs
                );
            }

            dep.pg().Execute(
                storages::postgres::ClusterHostType::kMaster,
                "INSERT INTO key_value_table(key, value) VALUES($1, $2) ON CONFLICT (key) DO UPDATE SET value = $2",
                key_value.key,
                key_value.value
            );

            return formats::json::Value{};
        });
}
