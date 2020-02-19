#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <yandex/taxi/yt-wrapper/ytlib.h>
#include <formats/json/value.hpp>

namespace storages::yt {

class Client;

/**********************************************/
/* Flags commonly accepted by Client commands */
/**********************************************/
namespace options {
struct Recursive {};
struct Force {};
struct Path {
  explicit Path(std::string path);
  Path(Path&& other) noexcept;
  std::string path_;
};
}  // namespace options

/**********************************************/
/*     Flags groups as command parameters     */
/**********************************************/
// TODO: move operation descriptors to Arcadia and expose
//       them via setters + FastPimpl
namespace operations {

class Exists {
  explicit Exists(const std::string path);
  Exists(Exists&& other) noexcept;

  void Set(options::Path path);

 private:
  friend class ::storages::yt::Client;
  std::string path_;
};

// clang-format off
/** Create directory node
  *
  * How recursive/force flags affect function behaviour when
  * trying to create "//foo/bar/baz"
  *
  * | Contents      | recursive | force  | result                            |
  * |:--------------|:---------:|:------:|:----------------------------------|
  * | //foo/bar     |     *     |    *   | baz created under //foo/bar       |
  * | //foo         |   false   |    *   | EXCEPTION                         |
  * | //foo         |   true    |    *   | bar/baz created under //foo       |
  * | //foo/bar/baz |     *     | false  | EXCEPTION                         |
  * | //foo/bar/baz |     *     | true   | OK, no changes made to cypress    |
  *
  * @param path node path to create
  * @param recursive also create parent directories of node.
  *        If false exception is raised if node's parent does not exist.
  * @param force don't raise an error if node already exist.
  *        If false exception is raised if directory with specified path
  *        already exist.
  */
// clang-format on
class CreateDirectory {
  explicit CreateDirectory(std::string path);
  CreateDirectory(CreateDirectory&& other) noexcept;

  void Set(options::Path path);
  void Set(const options::Recursive);
  void Set(const options::Force);

 private:
  friend class ::storages::yt::Client;
  std::string path_;
  bool recursive_;
  bool force_;
};

// clang-format off
/** Remove directory/file/table.
 *
 * How recursive/force flags affect function behaviour when
 * trying to remove "//foo/bar"
 *
 * | Contents      | recursive | force  | result                               |
 * |:--------------|:---------:|:------:|:-------------------------------------|
 * | //foo/bar     |     *     |    *   |  //foo/bar removed                   |
 * | //foo/bar/baz |   false   |    *   |  EXCEPTION                           |
 * | //foo/bar/baz |   true    |    *   |  //foo/bar/baz and //foo/bar removed |
 * | //foo         |     *     | false  |  EXCEPTION                           |
 * | //foo         |     *     | true   |  OK, no changes made to cypress      |
 *
 *
 * @param path node path to remove
 * @param recursive also remove contents of non-empty directories.
 *        If false exception is raised if target directory is not empty.
 * @param force ignore non-existing paths.
 *        If false exception is raised if path does not exist.
 */
// clang-format on
class Remove {
  explicit Remove(std::string path);
  Remove(Remove&& other) noexcept;

  void Set(options::Path path);
  void Set(const options::Recursive);
  void Set(const options::Force);

 private:
  friend class ::storages::yt::Client;
  std::string path_;
  bool recursive_;
  bool force_;
};
}  // namespace operations

/**********************************************/
/*         Core functions and classes         */
/**********************************************/
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

/** Client represents established authorized connection to specific Yt cluster.
 * Only one operation may be active on a single client at time.
 *
 * TODO: add timeout and other options. Expose async interface and ability to
 * abort running operation. */
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

  template <typename... Options>
  bool Exists(const std::string& path, Options&&... options);
  template <typename... Options>
  void CreateDirectory(const std::string& path, Options&&... options);
  template <typename... Options>
  void Remove(const std::string& path, Options&&... options);

  bool Execute(operations::Exists op);
  void Execute(operations::CreateDirectory op);
  void Execute(operations::Remove op);

 private:
  explicit Client(std::shared_ptr<NTaxi::YT::TClient> client);

  template <typename Operation, typename... Options>
  auto ExecuteOperation(Operation&& operation, Options&&... options);

  friend class Connection;
  std::shared_ptr<NTaxi::YT::TClient> client_;
};

template <typename Operation, typename... Options>
auto Client::ExecuteOperation(Operation&& operation, Options&&... options) {
  (operation.Set(std::forward<Options>(options)), ...);
  return Execute(std::forward<Operation>(operation));
}

template <typename... Options>
bool Client::Exists(const std::string& path, Options&&... options) {
  return ExecuteOperation(operations::Exists(path),
                          std::forward<Options>(options)...);
}

template <typename... Options>
void Client::CreateDirectory(const std::string& path, Options&&... options) {
  ExecuteOperation(operations::CreateDirectory(path),
                   std::forward<Options>(options)...);
}

template <typename... Options>
void Client::Remove(const std::string& path, Options&&... options) {
  ExecuteOperation(operations::Remove(path), std::forward<Options>(options)...);
}

/** Connection class represents intention to operate with some specified
 * cluster. One can use single instance of this class to acquire as many Client
 * instances as needed. */
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
