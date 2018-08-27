#pragma once

#include <yandex/taxi/userver/components/component_list.hpp>

namespace utils {

int DaemonMain(int argc, char** argv,
               const components::ComponentList& components_list);

}  // namespace utils
