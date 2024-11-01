#pragma once

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

struct TaskBase::Impl {
    boost::intrusive_ptr<impl::TaskContext> context;
};

}  // namespace engine

USERVER_NAMESPACE_END
