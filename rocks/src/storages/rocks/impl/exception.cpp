#include <userver/storages/rocks/impl/exception.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace rocks {

RequestFailedException::RequestFailedException(
    std::string_view request_description, std::string_view status)
    : Exception(fmt::format("{} request failed with status '{}'",
                            request_description, status)),
      status_(status) {}

std::string_view RequestFailedException::GetStatusString() const {
  return status_;
}

}  // namespace storages::rocks

USERVER_NAMESPACE_END
