#include <userver/storages/redis/bit_operation.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

std::string ToString(BitOperation bitop) {
  std::string operation{"ERR"};
  switch (bitop) {
    case BitOperation::kOr:
      operation = "OR";
      break;
    case BitOperation::kXor:
      operation = "XOR";
      break;
    case BitOperation::kAnd:
      operation = "AND";
      break;
    case BitOperation::kNot:
      operation = "NOT";
      break;
  }

  return operation;
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
