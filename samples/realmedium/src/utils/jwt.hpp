#pragma once

#include <string>

#include <jwt/jwt.hpp>

namespace real_medium::utils::jwt {

std::string GenerateJWT(std::string_view id);
::jwt::jwt_payload DecodeJWT(std::string_view jwt_token);

}  // namespace real_medium::utils::jwt
