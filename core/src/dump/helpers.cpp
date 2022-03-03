#include <userver/dump/helpers.hpp>

#include <fmt/format.h>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

void ThrowDumpUnimplemented(const std::string& name) {
  UINVARIANT(false, fmt::format("Dumps are unimplemented for {}. "
                                "See dump::Read, dump::Write",
                                name));
}

}  // namespace dump

USERVER_NAMESPACE_END
