#pragma once

#include <string>

#include <components/thread_pool.hpp>

#include "component_list.hpp"

namespace server {
namespace impl {

const std::string kIoThreadPoolName = "io-thread-pool";

const auto kServerComponentList =
    ComponentList().Append<components::ThreadPool>(kIoThreadPoolName);
}  // namespace impl
}  // namespace server
