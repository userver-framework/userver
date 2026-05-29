#include <userver/storages/scylla/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla {

ScyllaException::ScyllaException()
    : utils::TracefulException{utils::TracefulException::TraceMode::kIfLoggingIsEnabled}
{
    *this << "ScyllaDB error";
}

ScyllaException::ScyllaException(std::string_view what)
    : utils::TracefulException{utils::TracefulException::TraceMode::kIfLoggingIsEnabled}
{
    *this << what;
}

ServerException::ServerException(int code, std::string_view what)
    : QueryException{},
      code_(code)
{
    *this << "ScyllaDB server error [" << code << "]: " << what;
}

TimeoutException::TimeoutException(int code, std::string_view what, std::chrono::milliseconds timeout)
    : ServerException{code, {}},
      timeout_(timeout)
{
    *this << "ScyllaDB timeout [" << code << ", " << timeout.count() << "ms]: " << what;
}

CancelledException::CancelledException(ByDeadlinePropagation)
    : ScyllaException{},
      by_deadline_propagation_(true)
{
    *this << "ScyllaDB operation cancelled by upstream deadline propagation";
}

}  // namespace storages::scylla

USERVER_NAMESPACE_END
