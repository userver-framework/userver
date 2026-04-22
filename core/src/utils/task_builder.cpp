#include <userver/utils/task_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

TaskBuilder& TaskBuilder::Critical() {
    importance_ = engine::Task::Importance::kCritical;
    return *this;
}

TaskBuilder& TaskBuilder::SpanName(std::string&& name) {
    span_.emplace(std::move(name));
    return *this;
}

TaskBuilder& TaskBuilder::HideSpan() {
    span_.emplace(HideSpanTag{});
    return *this;
}

TaskBuilder& TaskBuilder::NoSpan() {
    span_.emplace(NoSpanTag{});
    return *this;
}

TaskBuilder& TaskBuilder::TaskProcessor(engine::TaskProcessor& tp) {
    tp_ = &tp;
    return *this;
}

TaskBuilder& TaskBuilder::Deadline(engine::Deadline deadline) {
    deadline_ = deadline;
    return *this;
}

TaskBuilder& TaskBuilder::Background() {
    inherit_variables_ = utils::impl::SpanWrapCall::InheritVariables::kNo;
    return *this;
}

}  // namespace utils

USERVER_NAMESPACE_END
