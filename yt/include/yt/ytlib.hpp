#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <yandex/taxi/yt-wrapper/ytlib.h>
#include <formats/json/value.hpp>

namespace storages::yt {

void GlobalInit(int yt_pool_size);
void GlobalShutdown();

class Row {
 public:
  Row();
  Row(const Row&) = delete;
  Row(Row&&) noexcept;
  ~Row();

  int Size() const;
  template <typename T>
  T Get(int index) const;

 private:
  friend class ReadCursor;
  NTaxi::YT::TUnversionedRow row_;
};

extern template std::int64_t Row::Get(int) const;
extern template std::uint64_t Row::Get(int) const;
extern template double Row::Get(int) const;
extern template bool Row::Get(int) const;
extern template std::string Row::Get(int) const;
// NOTE:
// 1) yson is not fully-convertable to json. Yson containing extension nodes,
//    attributes and so on will not be converted to json::Value
// 2) "null value" and "yson's null" are different things, so you may
//    need to call std::optional<f::j::Value> version of Get
extern template formats::json::Value Row::Get(int) const;
extern template std::optional<std::int64_t> Row::Get(int) const;
extern template std::optional<std::uint64_t> Row::Get(int) const;
extern template std::optional<double> Row::Get(int) const;
extern template std::optional<bool> Row::Get(int) const;
extern template std::optional<std::string> Row::Get(int) const;
extern template std::optional<formats::json::Value> Row::Get(int) const;

class ReadCursor {
 public:
  ReadCursor() = delete;
  ReadCursor(const ReadCursor&) = delete;
  ReadCursor(ReadCursor&&) noexcept;
  ~ReadCursor();

  // note: string-based data will become inaccessible once you call Next again
  bool Next(Row* row);

 private:
  friend class Client;
  ReadCursor(std::shared_ptr<NTaxi::YT::TReadCursor> cursor);
  std::shared_ptr<NTaxi::YT::TReadCursor> cursor_;
};

class Client {
 public:
  Client() = delete;
  Client(const Client&) = delete;
  Client(Client&&) noexcept;
  ~Client();

  ReadCursor ReadTable(
      const std::string& path, std::size_t batch_size, std::uint64_t from,
      std::uint64_t to,
      const std::optional<std::vector<std::string>>& columns = {});

 private:
  explicit Client(std::shared_ptr<NTaxi::YT::TClient> client);
  friend class Connection;
  std::shared_ptr<NTaxi::YT::TClient> client_;
};

class Connection {
 public:
  Connection() = delete;
  explicit Connection(const std::string& url);
  Connection(const Connection&) = delete;
  Connection(Connection&&) noexcept;
  ~Connection();

  Client CreateClient(const std::string& token);

 private:
  std::shared_ptr<NTaxi::YT::TConnection> conn_;
};

}  // namespace storages::yt
