#pragma once

#include <string>

#include "component_list.hpp"

namespace components {

void Run(const std::string& config_path, const ComponentList& component_list,
         const std::string& init_log_path = {});

void RunOnce(const std::string& config_path,
             const ComponentList& component_list,
             const std::string& init_log_path = {});

}  // namespace components
