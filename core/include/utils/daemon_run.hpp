#pragma once

#include <cache/cache_config.hpp>
#include <components/component_list.hpp>

namespace utils {

int DoDaemonMain(int argc, char** argv,
                 const components::ComponentList& components_list);

inline int DaemonMain(int argc, char** argv,
                      const components::ComponentList& components_list) {
  components::CacheConfigInit();
  return DoDaemonMain(argc, argv, components_list);
}

}  // namespace utils
