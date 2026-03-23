#include <userver/storages/scylla/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla {

ScyllaException::ScyllaException() : TracefulException("ScyllaDB error") {}

ScyllaException::ScyllaException(std::string_view what) : TracefulException(what) {}

}  // namespace storages::scylla

USERVER_NAMESPACE_END