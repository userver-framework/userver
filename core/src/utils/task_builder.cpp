#include <userver/utils/task_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

template class TaskBuilder<impl::TaskBuilderWithoutSelectedSpanOptions>;
template class TaskBuilder<impl::TaskBuilderWithSpanOptions>;
template class TaskBuilder<impl::TaskBuilderHideSpanOptions>;
template class TaskBuilder<impl::TaskBuilderNoSpanOptions>;

}  // namespace utils

USERVER_NAMESPACE_END
