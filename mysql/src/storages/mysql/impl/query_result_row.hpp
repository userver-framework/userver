#pragma once

#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

class QueryResultRow final {
 public:
  QueryResultRow();
  QueryResultRow(char** data, std::size_t fields_count,
                 std::size_t* fields_lengths);

  QueryResultRow(const QueryResultRow& other) = delete;
  QueryResultRow(QueryResultRow&& other) noexcept;

  ~QueryResultRow();

  std::size_t FieldsCount() const;

  const std::string& GetField(std::size_t ind) const;
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
