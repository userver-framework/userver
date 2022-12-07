#pragma once

#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

class MySQLRow final {
 public:
  MySQLRow();
  MySQLRow(char** data, std::size_t fields_count);

  MySQLRow(const MySQLRow& other) = delete;
  MySQLRow(MySQLRow&& other) noexcept;

  ~MySQLRow();

  std::size_t FieldsCount() const;

  std::string& GetField(std::size_t ind);

  auto begin() { return columns_.begin(); }
  auto begin() const { return columns_.begin(); }

  auto end() { return columns_.end(); }
  auto end() const { return columns_.end(); }

 private:
  std::vector<std::string> columns_;
};

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
