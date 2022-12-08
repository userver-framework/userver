#pragma once

#include <memory>
#include <string>
#include <vector>

#include <userver/engine/deadline.hpp>

#include <userver/storages/mysql/io/binder.hpp>
#include <userver/storages/mysql/io/extractor.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace infra {
class Pool;
}

class Cluster final {
 public:
  Cluster();
  ~Cluster();

  template <typename T, typename... Args>
  std::vector<T> Execute(engine::Deadline deadline, const std::string& query,
                         Args&&... args);

 private:
  void DoExecute(const std::string& query, io::ParamsBinderBase& params,
                 io::ExtractorBase& extractor, engine::Deadline deadline);

  std::vector<std::shared_ptr<infra::Pool>> pool_;
};

template <typename T, typename... Args>
std::vector<T> Cluster::Execute(engine::Deadline deadline,
                                const std::string& query, Args&&... args) {
  auto params_binder = io::BindParams(/* no forward here */ args...);
  auto extractor = io::TypedExtractor<T>{};

  DoExecute(query, params_binder, extractor, deadline);

  return std::move(extractor).ExtractData();
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
