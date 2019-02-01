#include <storages/mongo_ng/pool.hpp>

#include "pool_impl.hpp"

namespace storages {
namespace mongo_ng {

Pool::Pool(std::string id, const std::string& uri, const PoolConfig& config)
    : impl_(std::make_shared<impl::PoolImpl>(std::move(id), uri, config)) {}

Pool::~Pool() = default;

}  // namespace mongo_ng
}  // namespace storages
