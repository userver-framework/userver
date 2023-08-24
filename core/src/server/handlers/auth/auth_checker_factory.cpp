#include <userver/server/handlers/auth/auth_checker_factory.hpp>

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {
namespace {

class AuthCheckerFactories final {
 public:
  void RegisterFactory(std::string auth_type,
                       std::unique_ptr<AuthCheckerFactoryBase>&& factory);
  const AuthCheckerFactoryBase& GetFactory(const std::string& auth_type) const;

 private:
  std::unordered_map<std::string, std::unique_ptr<AuthCheckerFactoryBase>>
      factories_;
};

void AuthCheckerFactories::RegisterFactory(
    std::string auth_type, std::unique_ptr<AuthCheckerFactoryBase>&& factory) {
  auto res = factories_.emplace(std::move(auth_type), std::move(factory));
  if (!res.second) {
    throw std::runtime_error("can't register auth checker with type " +
                             res.first->first);
  }
}

const AuthCheckerFactoryBase& AuthCheckerFactories::GetFactory(
    const std::string& auth_type) const {
  try {
    return *factories_.at(auth_type);
  } catch (const std::exception& ex) {
    throw std::runtime_error("unknown type of auth checker: " + auth_type);
  }
}

AuthCheckerFactories& GetAuthCheckerFactories() {
  static AuthCheckerFactories auth_checker_factories;
  return auth_checker_factories;
}

}  // namespace

void RegisterAuthCheckerFactory(
    std::string auth_type, std::unique_ptr<AuthCheckerFactoryBase>&& factory) {
  GetAuthCheckerFactories().RegisterFactory(std::move(auth_type),
                                            std::move(factory));
}

const AuthCheckerFactoryBase& GetAuthCheckerFactory(
    const std::string& auth_type) {
  return GetAuthCheckerFactories().GetFactory(auth_type);
}

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
