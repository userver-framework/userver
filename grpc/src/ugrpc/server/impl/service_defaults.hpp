#pragma once

#include <string>
#include <vector>

#include <boost/optional/optional.hpp>

#include <userver/engine/task/task_processor_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

struct ServiceDefaults final {
    // using boost::optional to easily generalize to references
    boost::optional<engine::TaskProcessor&> task_processor;
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
