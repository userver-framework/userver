#pragma once

#include <vector>

#include <storages/mysql/impl/query_result_row.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

class QueryResult final {
 public:
  QueryResult();
  ~QueryResult();

  QueryResult(const QueryResult& other) = delete;
  QueryResult(QueryResult&& other) noexcept;

  void AppendRow(QueryResultRow&& row);

  std::size_t RowsCount() const;

  const QueryResultRow& GetRow(std::size_t ind) const;
  QueryResultRow& GetRow(std::size_t ind);

  auto begin() { return rows_.begin(); }
  auto begin() const { return rows_.begin(); }

  auto end() { return rows_.end(); }
  auto end() const { return rows_.end(); }

 private:
  std::vector<QueryResultRow> rows_;
};

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
