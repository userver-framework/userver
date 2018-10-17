#pragma once

#include <chrono>

namespace storages {
namespace postgres {

struct PoolOptions {
  std::chrono::duration idle_timeout;
  size_t pool_size;
};

}  // namespace postgres
}  // namespace storages
