#include <fmt/core.h>
#include <fmt/format.h>

/// [Postgres bitstring sample - component]
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/components/component.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/io/ip.hpp>
#include <userver/storages/postgres/io/macaddr.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/from_string.hpp>

namespace pg_service_template {

namespace {

struct InetNetworkV6;
struct InetNetworkV4;

template <typename T>
struct TemplateQueryInfo;

template <>
struct TemplateQueryInfo<userver::utils::ip::AddressV4> {
  using Address = userver::utils::ip::AddressV4;
  const std::function<std::string(const Address&)>
      address_to_string = userver::utils::ip::AddressV4ToString;
  const std::function<Address(const std::string&)>
      address_from_string = userver::utils::ip::AddressV4FromString;
  const std::string_view column_name = "ipv4";
};

template <>
struct TemplateQueryInfo<userver::utils::ip::AddressV6> {
  using Address = userver::utils::ip::AddressV6;
  const std::function<std::string(const Address&)>
      address_to_string = userver::utils::ip::AddressV6ToString;
  const std::function<Address(const std::string&)>
      address_from_string = userver::utils::ip::AddressV6FromString;
  const std::string_view column_name = "ipv6";
};

template <>
struct TemplateQueryInfo<userver::utils::ip::NetworkV4> {
  using Address = userver::utils::ip::NetworkV4;
  const std::function<std::string(const Address&)>
      address_to_string = userver::utils::ip::NetworkV4ToString;
  const std::function<Address(const std::string&)>
      address_from_string = userver::utils::ip::NetworkV4FromString;
  const std::string_view column_name = "netv4";
};

template <>
struct TemplateQueryInfo<userver::utils::ip::NetworkV6> {
  using Address = userver::utils::ip::NetworkV6;
  const std::function<std::string(const Address&)>
      address_to_string = userver::utils::ip::NetworkV6ToString;
  const std::function<Address(const std::string&)>
      address_from_string = userver::utils::ip::NetworkV6FromString;
  const std::string_view column_name = "netv6";
};

template <>
struct TemplateQueryInfo<userver::utils::macaddr::Macaddr> {
  using Address = userver::utils::macaddr::Macaddr;
  const std::function<std::string(const Address&)>
      address_to_string = userver::utils::macaddr::MacaddrToString;
  const std::function<Address(const std::string&)>
      address_from_string = userver::utils::macaddr::MacaddrFromString;
  const std::string_view column_name = "mac";
};

template <>
struct TemplateQueryInfo<userver::utils::macaddr::Macaddr8> {
  using Address = userver::utils::macaddr::Macaddr8;
  const std::function<std::string(const Address&)>
      address_to_string = userver::utils::macaddr::Macaddr8ToString;
  const std::function<Address(const std::string&)>
      address_from_string = userver::utils::macaddr::Macaddr8FromString;
  const std::string_view column_name = "mac8";
};

template <>
struct TemplateQueryInfo<InetNetworkV6> {
  using Address = userver::utils::ip::InetNetwork;
  const std::function<std::string(const Address&)>
      address_to_string = [](const Address& address){
        using namespace userver::utils::ip;
        return NetworkV6ToString(NetworkV6FromInetNetwork(address));
      };
  const std::function<Address(const std::string&)>
      address_from_string = [](const std::string& str) {
        using namespace userver::utils::ip;
        return NetworkV6ToInetNetwork(NetworkV6FromString(str));
      };
  const std::string_view column_name = "inet";
};

template <>
struct TemplateQueryInfo<InetNetworkV4> {
  using Address = userver::utils::ip::InetNetwork;
  const std::function<std::string(const Address&)>
      address_to_string = [](const Address& address){
        using namespace userver::utils::ip;
        return NetworkV4ToString(NetworkV4FromInetNetwork(address));
      };
  const std::function<Address(const std::string&)>
      address_from_string = [](const std::string& str) {
        using namespace userver::utils::ip;
        return NetworkV4ToInetNetwork(NetworkV4FromString(str));
      };
  const std::string_view column_name = "inet";
};

template <typename T>
userver::storages::postgres::ResultSet DoQuery(
    userver::storages::postgres::ClusterPtr pg_cluster, const int64_t id,
    const T& net_objects, const std::string_view db_column) {
  const auto q = fmt::format(R"~(
INSERT INTO example_schema.network(id, {})
VALUES($1, $2)
ON CONFLICT (id) DO UPDATE
SET {} = EXCLUDED.{}
RETURNING id, {}
)~",
                             db_column, db_column, db_column, db_column);

  try {
    return pg_cluster->Execute(
        userver::storages::postgres::ClusterHostType::kMaster, q, id,
        net_objects);
  } catch (std::exception& ex) {
    throw userver::server::handlers::InternalServerError{
        userver::server::handlers::ExternalBody{
            fmt::format("Query execution error: {}", ex.what())}};
  }
}

class Network final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-network";

  Network(const userver::components::ComponentConfig& config,
          const userver::components::ComponentContext& component_context)
      : HttpHandlerBase(config, component_context),
        pg_cluster_(
            component_context
                .FindComponent<userver::components::Postgres>("network-db_1")
                .GetCluster()) {}

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext&) const override {
    using namespace userver::utils::ip;
    using namespace userver::utils::macaddr;

    const auto id_str = request.GetArg("id");
    const auto id = userver::utils::FromString<int64_t>(id_str);

    const auto type_str = request.GetArg("type");
    const auto value_str = request.GetArg("value");

    if (type_str == "ipv4") {
      return QueryNetworkAddress<AddressV4>(id, value_str);
    } else if (type_str == "ipv6") {
      return QueryNetworkAddress<AddressV6>(id, value_str);
    } else if (type_str == "netv4") {
      return QueryNetworkAddress<NetworkV4>(id, value_str);
    } else if (type_str == "netv6") {
      return QueryNetworkAddress<NetworkV6>(id, value_str);
    } else if (type_str == "mac") {
      return QueryNetworkAddress<Macaddr>(id, value_str);
    } else if (type_str == "mac8") {
      return QueryNetworkAddress<Macaddr8>(id, value_str);
    } else if (type_str == "inet_v4") {
      return QueryNetworkAddress<InetNetworkV4>(id, value_str);
    } else if (type_str == "inet_v6") {
      return QueryNetworkAddress<InetNetworkV6>(id, value_str);
    } 
    else {
      throw userver::server::handlers::ClientError(
          userver::server::handlers::ExternalBody{"unsupported type"});
    }
  }

 private:
  template <typename T>
  std::string QueryNetworkAddress(int64_t id,
                                  const std::string& value_str) const {
    TemplateQueryInfo<T> query_info;
    using Address = typename TemplateQueryInfo<T>::Address;
    const auto addr = query_info.address_from_string(value_str);
    const auto res_query =
        DoQuery(pg_cluster_, id, addr, query_info.column_name);
    using Result = std::tuple<int64_t, Address>;
    auto [res_id, res] = res_query.template AsSingleRow<Result>(
        userver::storages::postgres::kRowTag);
    return fmt::format("id:{} {}:{}", res_id, query_info.column_name,
                       query_info.address_to_string(res));
  }

  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace



}  // namespace pg_service_template

/// [Postgres bitstring sample - main]
int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<pg_service_template::Network>()
          .Append<components::Postgres>("network-db_1")
          .Append<components::TestsuiteSupport>()
          .Append<clients::dns::Component>();
  return utils::DaemonMain(argc, argv, component_list);
}
/// [Postgres bitstring sample - main]
