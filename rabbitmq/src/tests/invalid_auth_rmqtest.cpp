#include "utils_rmqtest.hpp"

USERVER_NAMESPACE_BEGIN

UTEST(InvalidAuth, Basic) {
  clients::dns::Resolver resolver{engine::current_task::GetTaskProcessor(), {}};

  auto settings = urabbitmq::TestsHelper::CreateSettings();
  settings.use_secure_connection = false;
  settings.endpoints.auth.login = "itrofimow";
  settings.endpoints.auth.password = "NotMyPassw0rd";

  EXPECT_ANY_THROW(urabbitmq::Client::Create(resolver, settings));
}

USERVER_NAMESPACE_END
