#include <userver/crypto/exception.hpp>

#include <userver/compiler/impl/nodebug.hpp>

USERVER_NAMESPACE_BEGIN

namespace crypto {

// vtable anchor functions
USERVER_IMPL_NODEBUG CryptoException::~CryptoException() = default;
USERVER_IMPL_NODEBUG SignError::~SignError() = default;
USERVER_IMPL_NODEBUG VerificationError::~VerificationError() = default;
USERVER_IMPL_NODEBUG KeyParseError::~KeyParseError() = default;
USERVER_IMPL_NODEBUG SerializationError::~SerializationError() = default;

}  // namespace crypto

USERVER_NAMESPACE_END
