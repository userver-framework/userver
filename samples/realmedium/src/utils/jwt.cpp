#include "jwt.hpp"
#include <userver/logging/log.hpp>
namespace real_medium::utils::jwt {

using namespace ::jwt::params;

std::string GenerateJWT(std::string_view id) {
  ::jwt::jwt_object obj{algorithm("HS256"), secret("secret"),
                        payload({{"id", id.data()}})};
  return obj.signature();
}

::jwt::jwt_payload DecodeJWT(std::string_view jwt_token) {
  auto dec_obj = ::jwt::decode(jwt_token, algorithms({"HS256"}),
                               secret("secret"), verify(true));
  return dec_obj.payload();
}

}  // namespace real_medium::utils::jwt
