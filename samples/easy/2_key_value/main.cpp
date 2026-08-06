#include <userver/utest/using_namespace_userver.hpp>  // Note: this is for the purposes of samples only

#include <userver/easy.hpp>

constexpr std::string_view kSchema = R"~(
CREATE TABLE IF NOT EXISTS key_value_table (
  key VARCHAR PRIMARY KEY,
  value VARCHAR
)
)~";

int main(int argc, char* argv[]) {
    easy::HttpWith<easy::PgDep>(argc, argv)
        .DbSchema(kSchema)
        .Get(
            "/kv",
            [](const server::http::HttpRequest& req, const easy::PgDep& dep) {
                auto res = dep.pg().Execute(
                    storages::postgres::ClusterHostType::kSlave,
                    "SELECT value FROM key_value_table WHERE key=$1",
                    req.GetArg("key")
                );
                return res[0][0].As<std::string>();
            }
        )
        .Post(
            "/kv",
            [](const server::http::HttpRequest& req, const auto& dep) {
                dep.pg().Execute(
                    storages::postgres::ClusterHostType::kMaster,
                    "INSERT INTO key_value_table(key, value) VALUES($1, $2) ON CONFLICT (key) DO UPDATE SET value = $2",
                    req.GetArg("key"),
                    req.GetArg("value")
                );
                return std::string{};
            }
        )
        .DefaultContentType(http::content_type::kTextPlain);
}
