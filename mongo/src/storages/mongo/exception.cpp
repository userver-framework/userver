#include <userver/storages/mongo/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

MongoException::MongoException()
    : utils::TracefulException{
          utils::TracefulException::TraceMode::kIfLoggingIsEnabled} {}

MongoException::MongoException(std::string_view what) : MongoException{} {
  *this << what;
}

CancelledException::CancelledException(ByDeadlinePropagation)
    : by_deadline_propagation_(true) {
  *this << "Operation cancelled: deadline propagation";
}

bool CancelledException::IsByDeadlinePropagation() const {
  return by_deadline_propagation_;
}

}  // namespace storages::mongo

USERVER_NAMESPACE_END
