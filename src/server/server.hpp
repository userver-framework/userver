#pragma once

#include <string>

#include "component_list.hpp"

namespace server {

void Run(const std::string& config_path, const ComponentList& component_list,
         const std::string& init_log_path = {});

}  // namespace server
