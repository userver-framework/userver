#include <yt/ytlib.hpp>

#include <stdexcept>
#include <variant>

#include <engine/async.hpp>
#include <engine/standalone.hpp>
#include <engine/task/task_processor.hpp>
#include <formats/json/value.hpp>
#include <formats/json/value_builder.hpp>

#include <fmt/format.h>

namespace storages::yt {
namespace {
const char* Describe(NTaxi::YT::YT_EValueType t) {
  switch (t) {
    case NTaxi::YT::EValueType_Null:
      return "nullptr_t";
    case NTaxi::YT::EValueType_Int64:
      return "int64_t";
    case NTaxi::YT::EValueType_Uint64:
      return "uint64_t";
    case NTaxi::YT::EValueType_Double:
      return "double";
    case NTaxi::YT::EValueType_Boolean:
      return "bool";
    case NTaxi::YT::EValueType_String:
      return "string";
    case NTaxi::YT::EValueType_Any:
      return "yson";
    default:
      return "<unknown>";
  }
}

// hack: globally used task processor, initialized from GlobalInit()
std::unique_ptr<engine::impl::TaskProcessorHolder> gYtProcessor;
}  // namespace

/***************************************************/
/*                Yson->json stuff                 */
/***************************************************/
/* {{{ */
namespace {
class PrimitiveBuilder {
 public:
  void Accept(formats::json::ValueBuilder&& val);
  formats::json::ValueBuilder Extract();

 private:
  formats::json::ValueBuilder data;
};

void PrimitiveBuilder::Accept(formats::json::ValueBuilder&& val) {
  data = std::move(val);
}

formats::json::ValueBuilder PrimitiveBuilder::Extract() {
  return std::move(data);
}

class ArrayBuilder {
 public:
  ArrayBuilder();
  void Accept(formats::json::ValueBuilder&& val);
  formats::json::ValueBuilder Extract();

 private:
  formats::json::ValueBuilder array;
};

ArrayBuilder::ArrayBuilder() : array(formats::common::Type::kArray) {}

void ArrayBuilder::Accept(formats::json::ValueBuilder&& val) {
  array.PushBack(std::move(val));
}

formats::json::ValueBuilder ArrayBuilder::Extract() { return std::move(array); }

class MapBuilder {
 public:
  MapBuilder();

  void SetKey(const char* str, int len);
  void Accept(formats::json::ValueBuilder&& val);
  formats::json::ValueBuilder Extract();

 private:
  std::string key;
  formats::json::ValueBuilder object;
};

MapBuilder::MapBuilder() : object(formats::common::Type::kObject) {}

void MapBuilder::SetKey(const char* str, int len) { key.assign(str, len); }

void MapBuilder::Accept(formats::json::ValueBuilder&& val) {
  object[key] = std::move(val);
}

formats::json::ValueBuilder MapBuilder::Extract() { return std::move(object); }

class Builder {
 public:
  template <typename T>
  Builder(T&& val) : builder(std::move(val)) {}

  void Accept(formats::json::ValueBuilder&& val) {
    std::visit([&](auto& b) { b.Accept(std::move(val)); }, builder);
  }

  formats::json::ValueBuilder Extract() {
    return std::visit([](auto& b) { return b.Extract(); }, builder);
  }

  void SetKey(const char* data, int len) {
    std::visit(
        [data, len](auto& b) {
          using T = std::decay_t<decltype(b)>;
          if constexpr (std::is_same_v<T, MapBuilder>) {
            b.SetKey(data, len);
          }
        },
        builder);
  }

 private:
  using Variant = std::variant<PrimitiveBuilder, ArrayBuilder, MapBuilder>;
  Variant builder;
};

struct JsonBuilder : public NTaxi::YT::IYsonConsumer {
  JsonBuilder();
  formats::json::Value ExtractValue();

  bool AcceptString(const char* data, int len) noexcept override;
  bool AcceptInt64(int64_t val) noexcept override;
  bool AcceptUint64(uint64_t val) noexcept override;
  bool AcceptDouble(double val) noexcept override;
  bool AcceptBool(bool val) noexcept override;
  bool BeginMap() noexcept override;
  bool AcceptKey(const char* key, int len) noexcept override;
  bool EndMap() noexcept override;
  bool BeginList() noexcept override;
  bool EndList() noexcept override;

 private:
  std::vector<Builder> stack;
};

JsonBuilder::JsonBuilder() { stack.emplace_back(PrimitiveBuilder{}); }

bool JsonBuilder::AcceptString(const char* data, int len) noexcept {
  stack.back().Accept({std::string(data, len)});
  return true;
}

bool JsonBuilder::AcceptInt64(int64_t val) noexcept {
  stack.back().Accept({val});
  return true;
}

bool JsonBuilder::AcceptUint64(uint64_t val) noexcept {
  stack.back().Accept({val});
  return true;
}

bool JsonBuilder::AcceptDouble(double val) noexcept {
  stack.back().Accept({val});
  return true;
}

bool JsonBuilder::AcceptBool(bool val) noexcept {
  stack.back().Accept({val});
  return true;
}

bool JsonBuilder::BeginMap() noexcept {
  stack.emplace_back(MapBuilder{});
  return true;
}

bool JsonBuilder::EndMap() noexcept {
  formats::json::ValueBuilder object = stack.back().Extract();
  stack.pop_back();
  if (stack.empty()) return false;
  stack.back().Accept(std::move(object));
  return true;
}

bool JsonBuilder::AcceptKey(const char* key, int len) noexcept {
  stack.back().SetKey(key, len);
  return true;
}

bool JsonBuilder::BeginList() noexcept {
  stack.emplace_back(ArrayBuilder{});
  return true;
}

bool JsonBuilder::EndList() noexcept {
  formats::json::ValueBuilder array = stack.back().Extract();
  stack.pop_back();
  if (stack.empty()) return false;
  stack.back().Accept(std::move(array));
  return true;
}

formats::json::Value JsonBuilder::ExtractValue() {
  if (stack.size() != 1) throw std::runtime_error("Malformed yson");
  return stack.front().Extract().ExtractValue();
}
}  // namespace
/* }}} */

namespace {
using Yalue = NTaxi::YT::TUnversionedValue;
template <typename T>
T As(const Yalue&);

void typecheck(NTaxi::YT::YT_EValueType actual,
               NTaxi::YT::YT_EValueType expected) {
  if (actual != expected)
    throw std::runtime_error(fmt::format("got {} instead of {}",
                                         Describe(actual), Describe(expected)));
}

template <>
std::string As(const Yalue& val) {
  typecheck(val.type, NTaxi::YT::EValueType_String);
  return std::string(val.data.v_string, val.length);
}

template <>
std::int64_t As(const Yalue& val) {
  typecheck(val.type, NTaxi::YT::EValueType_Int64);
  return val.data.v_int64;
}

template <>
std::uint64_t As(const Yalue& val) {
  typecheck(val.type, NTaxi::YT::EValueType_Uint64);
  return val.data.v_uint64;
}

template <>
double As(const Yalue& val) {
  typecheck(val.type, NTaxi::YT::EValueType_Double);
  return val.data.v_double;
}

template <>
bool As(const Yalue& val) {
  typecheck(val.type, NTaxi::YT::EValueType_Boolean);
  return val.data.v_boolean;
}

template <>
formats::json::Value As(const Yalue& val) {
  typecheck(val.type, NTaxi::YT::EValueType_Any);
  JsonBuilder builder;
  if (!NTaxi::YT::ParseYsonBuf(val.data.v_string, val.length, &builder))
    throw std::runtime_error("Yson->Json conversion error");
  return builder.ExtractValue();
}

template <typename T>
struct get_as {
  T operator()(const NTaxi::YT::TUnversionedValue& val) const {
    return {As<T>(val)};
  }
};

template <typename T>
struct get_as<std::optional<T>> {
  std::optional<T> operator()(const NTaxi::YT::TUnversionedValue& val) const {
    if (val.type == NTaxi::YT::EValueType_Null) return std::nullopt;
    return {As<T>(val)};
  };
};

}  // namespace

void GlobalInit(int pool_size) {
  NTaxi::YT::GlobalInit();

  gYtProcessor = std::make_unique<engine::impl::TaskProcessorHolder>(
      engine::impl::TaskProcessorHolder::MakeTaskProcessor(
          pool_size, "yt-client",
          engine::current_task::GetTaskProcessor().GetTaskProcessorPools()));
}

void GlobalShutdown() { NTaxi::YT::GlobalShutdown(); }

/***************************************************/
/*                      Row                        */
/***************************************************/
Row::Row() = default;
Row::Row(Row&&) noexcept = default;
Row::~Row() = default;
int Row::Size() const { return row_.columns; }

template std::int64_t Row::Get(int) const;
template std::uint64_t Row::Get(int) const;
template double Row::Get(int) const;
template bool Row::Get(int) const;
template std::string Row::Get(int) const;
template formats::json::Value Row::Get(int) const;

template <typename T>
T Row::Get(int index) const {
  if (index < 0 || index >= Size()) throw std::runtime_error("out of bounds");
  return get_as<T>()(row_.column[index]);
}

template std::optional<std::int64_t> Row::Get(int) const;
template std::optional<std::uint64_t> Row::Get(int) const;
template std::optional<double> Row::Get(int) const;
template std::optional<bool> Row::Get(int) const;
template std::optional<std::string> Row::Get(int) const;
template std::optional<formats::json::Value> Row::Get(int) const;

/***************************************************/
/*                   ReadCursor                    */
/***************************************************/
ReadCursor::ReadCursor(ReadCursor&&) = default;
ReadCursor::~ReadCursor() = default;
ReadCursor::ReadCursor(std::shared_ptr<NTaxi::YT::TReadCursor> cursor)
    : cursor_(cursor) {}

bool ReadCursor::Next(Row* row) {
  for (;;) {
    auto status = cursor_->Next(&row->row_).value();
    switch (status) {
      case NTaxi::YT::TReadCursor::Ok:
        return true;
      case NTaxi::YT::TReadCursor::Wait:
        engine::impl::Async(
            *gYtProcessor->Get(),
            [cursor = cursor_]() { return cursor->WaitForData(); })
            .Get()
            .value();
        break;
      case NTaxi::YT::TReadCursor::Eof:
        return false;
    }
  }
}

/***************************************************/
/*                     Client                      */
/***************************************************/
Client::Client(Client&&) = default;
Client::~Client() = default;
Client::Client(std::shared_ptr<NTaxi::YT::TClient> client) : client_(client) {}

ReadCursor Client::ReadTable(
    const std::string& path, std::size_t batch_size, std::uint64_t from,
    std::uint64_t to, const std::optional<std::vector<std::string>>& columns) {
  return ReadCursor(
      engine::impl::Async(
          *gYtProcessor->Get(),
          [client = client_, path, batch_size, from, to, columns]() {
            std::vector<const char*> cols;
            if (columns.has_value() && !columns.value().empty()) {
              cols.reserve(columns.value().size());
              std::transform(columns.value().begin(), columns.value().end(),
                             std::back_inserter(cols),
                             [](const std::string& s) { return s.c_str(); });
            }
            return std::make_shared<NTaxi::YT::TReadCursor>(
                std::move(client
                              ->ReadTable(path.c_str(), batch_size, from, to,
                                          cols.empty() ? nullptr : cols.data(),
                                          cols.size())
                              .value()));
          })
          .Get());
}

/***************************************************/
/*                   Connection                    */
/***************************************************/
Connection::Connection(Connection&&) = default;
Connection::~Connection() = default;

Connection::Connection(const std::string& url)
    : conn_(engine::impl::Async(
                *gYtProcessor->Get(),
                [url]() {
                  return std::make_shared<NTaxi::YT::TConnection>(std::move(
                      NTaxi::YT::TConnection::Create(url.c_str()).value()));
                })
                .Get()) {}

Client Connection::CreateClient(const std::string& token) {
  return Client(engine::impl::Async(
                    *gYtProcessor->Get(),
                    [conn = conn_, token]() {
                      return std::make_shared<NTaxi::YT::TClient>(
                          std::move(conn->CreateClient(token.c_str()).value()));
                    })
                    .Get());
}
}  // namespace storages::yt
