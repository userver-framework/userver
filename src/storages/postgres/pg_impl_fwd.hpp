#pragma once

#include <memory>

namespace storages {
namespace postgres {

class Connection;
using ConnectionPtr = std::shared_ptr<Connection>;

}  // namespace postgres
}  // namespace storages
