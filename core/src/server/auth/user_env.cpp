#include <userver/server/auth/user_env.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::auth {

std::string ToString(UserEnv env) {
  switch (env) {
    case UserEnv::kProd:
      return "Prod";
    case UserEnv::kTest:
      return "Test";
    case UserEnv::kProdYateam:
      return "ProdYateam";
    case UserEnv::kTestYateam:
      return "TestYateam";
    case UserEnv::kStress:
      return "Stress";
  }

  UASSERT_MSG(false, "Unknown user env");
  throw std::runtime_error("Unknown user env " +
                           std::to_string(static_cast<int>(env)));
}

}  // namespace server::auth

USERVER_NAMESPACE_END
