#pragma once

#include <stdexcept>
#include <string>

#include <userver/formats/parse/to.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {
class AuthCheckerBase;
}

namespace server::auth {

/// @brief Authorization environments for users
enum class UserEnv : int {
  kProd,
  kTest,
  kProdYateam,
  kTestYateam,
  kStress,
};

template <class Value>
UserEnv Parse(const Value& v, formats::parse::To<UserEnv>) {
  const std::string env_name = v.template As<std::string>();
  if (env_name == "Prod" || env_name == "Mimino") {
    return UserEnv::kProd;
  } else if (env_name == "Test") {
    return UserEnv::kTest;
  } else if (env_name == "ProdYateam") {
    return UserEnv::kProdYateam;
  } else if (env_name == "TestYateam") {
    return UserEnv::kTestYateam;
  } else if (env_name == "Stress") {
    return UserEnv::kStress;
  }

  UASSERT_MSG(false, "Unknown user env");
  throw std::runtime_error("Unknown user env " + env_name);
}

std::string ToString(UserEnv env);

}  // namespace server::auth

USERVER_NAMESPACE_END
