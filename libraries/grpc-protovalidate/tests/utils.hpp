#include <buf/validate/validate.pb.h>

#include <optional>

#include <userver/ugrpc/client/exceptions.hpp>

#include "types/constrained.pb.h"

USERVER_NAMESPACE_BEGIN

namespace tests {

types::ConstrainedMessage CreateValidMessage(int32_t required_value);
types::ConstrainedMessage CreateInvalidMessage();
std::optional<buf::validate::Violations> GetViolations(const ugrpc::client::InvalidArgumentError& err);

}  // namespace tests

USERVER_NAMESPACE_END
