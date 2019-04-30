#pragma once

#include <cache/cache_config.hpp>
#include <components/component_list.hpp>

namespace server::handlers::auth::apikey {
extern int auth_checker_apikey_module_activation;
}  // namespace server::handlers::auth::apikey

namespace utils {

int DoDaemonMain(int argc, char** argv,
                 const components::ComponentList& components_list);

inline int DaemonMain(int argc, char** argv,
                      const components::ComponentList& components_list) {
  ++server::handlers::auth::apikey::auth_checker_apikey_module_activation;
  cache::CacheConfigInit();
  return DoDaemonMain(argc, argv, components_list);
}

}  // namespace utils
