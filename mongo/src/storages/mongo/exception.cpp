#include <userver/storages/mongo/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

MongoException::MongoException()
    : utils::TracefulException{
          utils::TracefulException::TraceMode::kIfLoggingIsEnabled} {}

MongoException::MongoException(std::string_view what) : MongoException{} {
  *this << what;
}

}  // namespace storages::mongo

USERVER_NAMESPACE_END
