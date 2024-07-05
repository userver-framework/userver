#include "log_helper_impl.hpp"

USERVER_NAMESPACE_BEGIN

namespace logging {

// Encode string for json format
void EncodeJson(LogBuffer& lb, std::string_view string);

}  // namespace logging

USERVER_NAMESPACE_END
