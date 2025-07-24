#include <userver/chaotic/io/crypto/base64/string64.hpp>

#include <userver/crypto/base64.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::convert {

::crypto::base64::String64 Convert(const std::string& str, chaotic::convert::To<::crypto::base64::String64>) {
    return ::crypto::base64::String64(crypto::base64::Base64Decode(str));
}

std::string Convert(const ::crypto::base64::String64& str64, chaotic::convert::To<std::string>) {
    return crypto::base64::Base64Encode(str64.GetUnderlying());
}

}  // namespace chaotic::convert

USERVER_NAMESPACE_END
