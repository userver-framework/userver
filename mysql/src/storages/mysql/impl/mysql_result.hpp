#pragma once

#include <vector>

#include <storages/mysql/impl/mysql_row.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

class MySQLResult final {
 public:
  MySQLResult();
  ~MySQLResult();

  MySQLResult(const MySQLResult& other) = delete;
  MySQLResult(MySQLResult&& other) noexcept;

  void AppendRow(MySQLRow&& row);

  std::size_t RowsCount() const;

  MySQLRow& GetRow(std::size_t ind);

  auto begin() { return rows_.begin(); }
  auto begin() const { return rows_.begin(); }

  auto end() { return rows_.end(); }
  auto end() const { return rows_.end(); }

 private:
  std::vector<MySQLRow> rows_;
};

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
