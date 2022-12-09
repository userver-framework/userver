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

namespace infra {
class Topology;
}

class Cluster final {
 public:
  Cluster();
  ~Cluster();

  template <typename... Args>
  StatementResultSet Execute(ClusterHostType host_type,
                             engine::Deadline deadline,
                             const std::string& query, Args&&... args);

  template <typename T>
  void InsertSingle(engine::Deadline deadline, const std::string& insert_query,
                    T&& row);

  template <typename Container>
  void InsertMany(engine::Deadline deadline, const std::string& insert_query,
                  Container&& rows);

 private:
  StatementResultSet DoExecute(ClusterHostType host_type,
                               const std::string& query,
                               io::ParamsBinderBase& params,
                               engine::Deadline deadline);

  void DoInsert(const std::string& insert_query, io::ParamsBinderBase& params,
                std::size_t rows_count, engine::Deadline deadline);

  utils::FastPimpl<infra::Topology, 24, 8> topology_;
};

template <typename... Args>
StatementResultSet Cluster::Execute(ClusterHostType host_type,
                                    engine::Deadline deadline,
                                    const std::string& query, Args&&... args) {
  auto params_binder =
      io::ParamsBinder::BindParams(/* no forward here */ args...);

  return DoExecute(host_type, query, params_binder, deadline);
}

/*template <typename T>
void Cluster::InsertSingle(engine::Deadline deadline,
                           const std::string& insert_query, T&& row) {
  // TODO : reuse ParamsBinder somehow
}*/

template <typename Container>
void Cluster::InsertMany(engine::Deadline deadline,
                         const std::string& insert_query, Container&& rows) {
  static_assert(io::kIsRange<Container>, "The type isn't actually a container");
  if (rows.empty()) {
    // TODO : throw?
    return;
  }

  io::InsertBinder binder{rows};
  binder.BindRows();

  DoInsert(insert_query, binder, rows.size(), deadline);
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
