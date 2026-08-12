#include <userver/dump/operations.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

void Reader::BackUp(std::size_t /*size*/) {
    UASSERT_MSG(false, "BackUp operation is not implemented");
    throw Error("BackUp operation is not implemented");
}

}  // namespace dump

USERVER_NAMESPACE_END
