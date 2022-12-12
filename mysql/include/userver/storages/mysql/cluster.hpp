#pragma once

#include <memory>
#include <string>
#include <vector>

#include <userver/engine/deadline.hpp>
#include <userver/utils/fast_pimpl.hpp>

#include <userver/storages/mysql/cluster_host_type.hpp>
#include <userver/storages/mysql/io/binder.hpp>
#include <userver/storages/mysql/io/insert_binder.hpp>
#include <userver/storages/mysql/statement_result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace tests {
class TestsHelper;
}

namespace settings {
class HostSettings;
}

namespace infra {
class Topology;
}

class Cluster final {
 public:
  Cluster(std::vector<settings::HostSettings>&& settings);
  ~Cluster();

  // An alias for Execute
  template <typename... Args>
  StatementResultSet Select(ClusterHostType host_type,
                            engine::Deadline deadline, const std::string& query,
                            const Args&... args) const;

  template <typename... Args>
  StatementResultSet Execute(ClusterHostType host_type,
                             engine::Deadline deadline,
                             const std::string& query,
                             const Args&... args) const;

  template <typename T>
  void InsertOne(engine::Deadline deadline, const std::string& insert_query,
                 T&& row) const;

  // only works with recent enough MariaDB as a server,
  // requires mariadb client lib
  template <typename Container>
  void InsertMany(engine::Deadline deadline, const std::string& insert_query,
                  Container&& rows, bool throw_on_empty_insert = true) const;

 private:
  StatementResultSet DoExecute(ClusterHostType host_type,
                               const std::string& query,
                               io::ParamsBinderBase& params,
                               engine::Deadline deadline) const;

  void DoInsert(const std::string& insert_query, io::ParamsBinderBase& params,
                engine::Deadline deadline) const;

  friend class tests::TestsHelper;
  std::string EscapeString(std::string_view source) const;

  utils::FastPimpl<infra::Topology, 24, 8> topology_;
};

template <typename... Args>
StatementResultSet Cluster::Select(ClusterHostType host_type,
                                   engine::Deadline deadline,
                                   const std::string& query,
                                   const Args&... args) const {
  return Execute(host_type, deadline, query, args...);
}

template <typename... Args>
StatementResultSet Cluster::Execute(ClusterHostType host_type,
                                    engine::Deadline deadline,
                                    const std::string& query,
                                    const Args&... args) const {
  auto params_binder = io::ParamsBinder::BindParams(args...);

  return DoExecute(host_type, query, params_binder, deadline);
}

template <typename T>
void Cluster::InsertOne(engine::Deadline deadline,
                        const std::string& insert_query, T&& row) const {
  // TODO : reuse DetectIsSuitableRowType from PG
  using Row = std::decay_t<T>;
  static_assert(boost::pfr::tuple_size_v<Row> != 0,
                "Row to insert has zero columns");

  auto binder = std::apply(
      [](auto&&... args) {
        return io::ParamsBinder::BindParams(
            std::forward<decltype(args)>(args)...);
      },
      boost::pfr::structure_tie(row));

  return DoInsert(insert_query, binder, deadline);
}

template <typename Container>
void Cluster::InsertMany(engine::Deadline deadline,
                         const std::string& insert_query, Container&& rows,
                         bool throw_on_empty_insert) const {
  if (rows.empty()) {
    if (throw_on_empty_insert) {
      throw std::runtime_error{"Empty insert requested"};
    } else {
      return;
    }
  }

  io::InsertBinder binder{rows};
  binder.BindRows();

  DoInsert(insert_query, binder, deadline);
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
