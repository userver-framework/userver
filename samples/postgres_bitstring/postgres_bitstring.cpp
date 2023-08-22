#include <userver/clients/dns/component.hpp>
#include <userver/storages/postgres/io/bitstring.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/from_string.hpp>

#include <userver/utest/using_namespace_userver.hpp>

/// [Postgres bitstring sample - component]
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

namespace samples::pg {

namespace {

enum class TestFlag {
  kNone = 0,
  k1 = 0x1,
  k2 = 0x2,
  k3 = 0x4,
  k4 = 0x8,
  k5 = 0x10,
  k6 = 0x20,
  k7 = 0x40,
  k8 = 0x80,
};

constexpr std::string_view kCppTypeKey = "cpp_type";
constexpr std::string_view kFlags8Type = "flags8";
constexpr std::string_view kUInt8Type = "uint8";
constexpr std::string_view kUInt32Type = "uint32";
constexpr std::string_view kBitset8Type = "bitset8";
constexpr std::string_view kBitset32Type = "bitset32";
constexpr std::string_view kBitset128Type = "bitset128";

constexpr std::string_view kDbTypeKey = "db_type";
constexpr std::string_view kVarbitType = "varbit";
constexpr std::string_view kBit8Type = "bit8";
constexpr std::string_view kBit32Type = "bit32";
constexpr std::string_view kBit128Type = "bit128";

bool IsBitVaryingType(const std::string_view db_type_str) {
  return db_type_str == kVarbitType;
}

bool IsBitType(const std::string_view db_type_str) {
  return db_type_str == kBit8Type || db_type_str == kBit32Type ||
         db_type_str == kBit128Type;
}

template <typename T, typename Enable = void>
struct BitStringConverter;

template <typename T>
struct BitStringConverter<T, std::enable_if_t<std::is_integral_v<T>>> {
  static T FromString(std::string_view bits_str) {
    T result = 0;
    for (size_t i = 0; (i < sizeof(T) * 8) && !bits_str.empty(); ++i) {
      if (bits_str.front() != '1' && bits_str.front() != '0') {
        throw server::handlers::ClientError{
            server::handlers::ExternalBody{"Invalid bit string format"}};
      }
      result |= ((bits_str.front() == '1' ? 1ull : 0ull) << i);
      bits_str.remove_prefix(1);
    }
    return result;
  }

  static std::string ToString(const T bits) {
    static_assert(std::is_integral_v<T>);

    std::string result;
    for (size_t i = 0; i < sizeof(bits) * 8; ++i) {
      result.push_back((bits & (1ull << i)) ? '1' : '0');
    }
    return result;
  }
};

template <size_t N>
struct BitStringConverter<std::bitset<N>> {
  static std::bitset<N> FromString(std::string bits_str) {
    // NOTE: bitset accepts bitstring in msb to lsb order
    std::reverse(bits_str.begin(), bits_str.end());
    try {
      return std::bitset<N>{bits_str};
    } catch (const std::exception& ex) {
      throw server::handlers::ClientError{
          server::handlers::ExternalBody{"Invalid bit string format"}};
    }
    return {};
  }

  static std::string ToString(const std::bitset<N>& bits) {
    // NOTE: bitset returns bitstring in msb to lsb order
    auto result = bits.to_string();
    std::reverse(result.begin(), result.end());
    return result;
  }
};

template <>
struct BitStringConverter<utils::Flags<TestFlag>> {
  static utils::Flags<TestFlag> FromString(std::string_view bits_str) {
    utils::Flags<TestFlag> result{};
    const auto value =
        BitStringConverter<utils::Flags<TestFlag>::ValueType>::FromString(
            bits_str);
    result.SetValue(value);
    return result;
  }

  static std::string ToString(const utils::Flags<TestFlag> bits) {
    // NOTE: bitset returns bitstring in msb to lsb order
    return BitStringConverter<utils::Flags<TestFlag>::ValueType>::ToString(
        bits.GetValue());
  }
};

}  // namespace

class BitStringHandler final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-bitstring";

  BitStringHandler(const components::ComponentConfig& config,
                   const components::ComponentContext& context);

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override;

 private:
  std::string GetValue(int64_t id,
                       const server::http::HttpRequest& request) const;

  std::string PostValue(int64_t id,
                        const server::http::HttpRequest& request) const;
            
  storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace samples::pg
/// [Postgres bitstring sample - component]

namespace samples::pg {

/// [Postgres bitstring sample - component constructor]
BitStringHandler::BitStringHandler(const components::ComponentConfig& config,
                                   const components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(
          context.FindComponent<components::Postgres>("bitstring-db_1")
              .GetCluster()) {
  using storages::postgres::ClusterHostType;
}
/// [Postgres bitstring sample - component constructor]

/// [Postgres bitstring sample - HandleRequestThrow]
std::string BitStringHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&) const {
  const auto& id_str = request.GetArg("id");
  if (id_str.empty()) {
    throw server::handlers::ClientError(
        server::handlers::ExternalBody{"No 'id' query argument"});
  }
  const auto id = utils::FromString<int64_t>(id_str);

  switch (request.GetMethod()) {
    case server::http::HttpMethod::kGet:
      return GetValue(id, request);
    case server::http::HttpMethod::kPost:
      return PostValue(id, request);
    default:
      throw server::handlers::ClientError(server::handlers::ExternalBody{
          fmt::format("Unsupported method {}", request.GetMethod())});
  }
}
/// [Postgres bitstring sample - HandleRequestThrow]

std::string SelectValueQuery(std::string_view db_column) {
  return fmt::format("SELECT {} FROM bitstring_table WHERE id=$1", db_column);
};

[[noreturn]] void ThrowUnsupportedDbType(const std::string_view db_type_str) {
  throw server::handlers::ClientError{server::handlers::ExternalBody{
      fmt::format("Unsupported {} {}", kDbTypeKey, db_type_str)}};
}
[[noreturn]] void ThrowUnsupportedCppType(const std::string_view cpp_type_str) {
  throw server::handlers::ClientError{server::handlers::ExternalBody{
      fmt::format("Unsupported {} {}", kCppTypeKey, cpp_type_str)}};
}

template <typename T>
std::string ParseData(storages::postgres::ResultSet result_set,
                      const std::string_view db_type) {
  if (IsBitType(db_type)) {
    const auto wrap =
        result_set.AsSingleRow<storages::postgres::BitStringWrapper<
            T, storages::postgres::BitStringType::kBit>>();
    return BitStringConverter<T>::ToString(wrap.bits);
  }
  if (IsBitVaryingType(db_type)) {
    const auto wrap =
        result_set.AsSingleRow<storages::postgres::BitStringWrapper<
            T, storages::postgres::BitStringType::kBitVarying>>();
    return BitStringConverter<T>::ToString(wrap.bits);
  }
  ThrowUnsupportedDbType(db_type);
}

template <size_t N>
std::bitset<N> ParseData(storages::postgres::ResultSet result_set,
                         const std::string_view db_type) {
  if (IsBitType(db_type)) {
    const auto wrap =
        result_set.AsSingleRow<storages::postgres::BitStringWrapper<
            std::bitset<N>, storages::postgres::BitStringType::kBit>>();
    return BitStringConverter<std::bitset<N>>::ToString(wrap.bits);
  }
  if (IsBitVaryingType(db_type)) {
    const auto bits = result_set.AsSingleRow<std::bitset<N>>();
    return BitStringConverter<std::bitset<N>>::ToString(bits);
  }
  ThrowUnsupportedDbType(db_type);
}

/// [Postgres bitstring sample - GetValue]
std::string BitStringHandler::GetValue(
    int64_t id, const server::http::HttpRequest& request) const {
  const auto& db_type_str = request.GetArg("db_type");
  const auto& cpp_type_str = request.GetArg("cpp_type");
  const storages::postgres::ResultSet result_set =
      pg_cluster_->Execute(storages::postgres::ClusterHostType::kSlave,
                           SelectValueQuery(db_type_str), id);
  if (result_set.IsEmpty()) {
    request.SetResponseStatus(server::http::HttpStatus::kNotFound);
    return {};
  }
  if (cpp_type_str == kFlags8Type) {
    return ParseData<utils::Flags<TestFlag>>(result_set, db_type_str);
  }
  if (cpp_type_str == kUInt8Type) {
    return ParseData<uint8_t>(result_set, db_type_str);
  }
  if (cpp_type_str == kUInt32Type) {
    return ParseData<uint32_t>(result_set, db_type_str);
  }
  if (cpp_type_str == kBitset8Type) {
    return ParseData<std::bitset<8>>(result_set, db_type_str);
  }
  if (cpp_type_str == kBitset32Type) {
    return ParseData<std::bitset<32>>(result_set, db_type_str);
  }
  if (cpp_type_str == kBitset128Type) {
    return ParseData<std::bitset<128>>(result_set, db_type_str);
  }
  ThrowUnsupportedCppType(cpp_type_str);
}
/// [Postgres bitstring type sample - GetValue]

std::string InsertValueQuery(std::string_view db_column) {
  return fmt::format(R"~(
INSERT INTO bitstring_table(id, {})
VALUES($1, $2)
ON CONFLICT (id) DO UPDATE
SET {} = EXCLUDED.{}
RETURNING {}
)~",
                     db_column, db_column, db_column, db_column);
};

template <typename T>
std::string ExecutePostQuery(storages::postgres::ClusterPtr pg_cluster,
                             const std::string_view db_type, int64_t id,
                             const T& value) {
  T result{};
  if (IsBitType(db_type)) {
    auto res = pg_cluster->Execute(storages::postgres::ClusterHostType::kMaster,
                                   InsertValueQuery(db_type), id,
                                   storages::postgres::Bit(value));
    res[0][0].To(storages::postgres::Bit(result));
  } else if (IsBitVaryingType(db_type)) {
    auto res = pg_cluster->Execute(storages::postgres::ClusterHostType::kMaster,
                                   InsertValueQuery(db_type), id,
                                   storages::postgres::Varbit(value));
    res[0][0].To(storages::postgres::Varbit(result));
  } else {
    ThrowUnsupportedDbType(db_type);
  }
  return BitStringConverter<T>::ToString(result);
}

template <size_t N>
std::string ExecutePostQuery(storages::postgres::ClusterPtr pg_cluster,
                             const std::string_view db_type, int64_t id,
                             const std::bitset<N>& value) {
  std::bitset<N> result;
  if (IsBitType(db_type)) {
    auto res = pg_cluster->Execute(storages::postgres::ClusterHostType::kMaster,
                                   InsertValueQuery(db_type), id,
                                   storages::postgres::Bit(value));
    res[0][0].To(storages::postgres::Bit(result));
  } else if (IsBitVaryingType(db_type)) {
    auto res = pg_cluster->Execute(storages::postgres::ClusterHostType::kMaster,
                                   InsertValueQuery(db_type), id, value);
    res[0][0].To(result);
  } else {
    ThrowUnsupportedDbType(db_type);
  }
  return BitStringConverter<std::bitset<N>>::ToString(result);
}

/// [Postgres bitstring sample - PostValue]
std::string BitStringHandler::PostValue(
    int64_t id, const server::http::HttpRequest& request) const {
  const auto& db_type_str = request.GetArg("db_type");
  const auto& cpp_type_str = request.GetArg("cpp_type");
  const auto& value_str = request.GetArg("value");

  if (cpp_type_str == kFlags8Type) {
    return ExecutePostQuery(
        pg_cluster_, db_type_str, id,
        BitStringConverter<utils::Flags<TestFlag>>::FromString(value_str));
  }
  if (cpp_type_str == kUInt8Type) {
    return ExecutePostQuery(pg_cluster_, db_type_str, id,
                            BitStringConverter<uint8_t>::FromString(value_str));
  }
  if (cpp_type_str == kUInt32Type) {
    return ExecutePostQuery(
        pg_cluster_, db_type_str, id,
        BitStringConverter<uint32_t>::FromString(value_str));
  }
  if (cpp_type_str == kBitset8Type) {
    return ExecutePostQuery(
        pg_cluster_, db_type_str, id,
        BitStringConverter<std::bitset<8>>::FromString(value_str));
  }
  if (cpp_type_str == kBitset32Type) {
    return ExecutePostQuery(
        pg_cluster_, db_type_str, id,
        BitStringConverter<std::bitset<32>>::FromString(value_str));
  }
  if (cpp_type_str == kBitset128Type) {
    return ExecutePostQuery(
        pg_cluster_, db_type_str, id,
        BitStringConverter<std::bitset<128>>::FromString(value_str));
  }
  ThrowUnsupportedCppType(cpp_type_str);
}
/// [Postgres bitstring sample - PostValue]

}  // namespace samples::pg

/// [Postgres bitstring sample - main]
int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<samples::pg::BitStringHandler>()
          .Append<components::Postgres>("bitstring-db_1")
          .Append<components::TestsuiteSupport>()
          .Append<clients::dns::Component>();
  return utils::DaemonMain(argc, argv, component_list);
}
/// [Postgres bitstring sample - main]
