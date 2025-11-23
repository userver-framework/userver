#include <userver/storages/rocks/exception.hpp>
#include <rocksdb/status.h>
#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

namespace detail {

void CheckStatus(const rocksdb::Status& status, std::string_view description) {
    if (!status.ok()) {
        throw USERVER_NAMESPACE::storages::rocks::StatusNokException(description, status.ToString());
    }
}

}  // namespace detail

StatusNokException::StatusNokException(std::string_view description, std::string_view status)
    : std::runtime_error(fmt::format("{} request failed with status '{}'", description, status)), status_(status) {}

std::string_view StatusNokException::GetStatusString() const { return status_; }

}  // namespace storages::rocks

USERVER_NAMESPACE_END
