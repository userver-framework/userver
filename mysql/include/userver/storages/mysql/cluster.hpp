#pragma once

#include <memory>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace infra {
class Pool;
}

class Cluster final {
 public:
  Cluster();
  ~Cluster();

 private:
  std::vector<std::shared_ptr<infra::Pool>> pool_;
};

}  // namespace storages::mysql

USERVER_NAMESPACE_END
