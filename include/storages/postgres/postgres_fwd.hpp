#pragma once

#include <memory>

namespace storages {
namespace postgres {

class Transaction;
class ResultSet;

class Cluster;
using ClusterPtr = std::shared_ptr<Cluster>;

}  // namespace postgres
}  // namespace storages
