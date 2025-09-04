#pragma once

#include <userver/chaotic/convert.hpp>
#include <userver/chaotic/io/userver/utils/strong_typedef.hpp>

#include <string>

namespace crypto::base64 {

// RFC4648
class String64 : public USERVER_NAMESPACE::utils::StrongTypedef<String64, std::string> {
    using StrongTypedef::StrongTypedef;
};

}  // namespace crypto::base64

USERVER_NAMESPACE_BEGIN

namespace chaotic::convert {

crypto::base64::String64 Convert(const std::string& str, chaotic::convert::To<crypto::base64::String64>);

std::string Convert(const crypto::base64::String64& str64, chaotic::convert::To<std::string>);

}  // namespace chaotic::convert

USERVER_NAMESPACE_END
