#include <userver/ydb/topic_writer_types.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

TopicWriteResult::TopicWriteResult(TopicWriteStatus status)
    : status_{status}
{}

TopicWriteResult::TopicWriteResult(
    utils::impl::InternalTag,
    TopicWriteStatus status,
    std::shared_ptr<impl::TopicWriterHandlingResult> context
)
    : status_{status},
      context_{std::move(context)}
{}

std::string_view ToStringView(TopicWriteStatus status) {
    switch (status) {
        case TopicWriteStatus::kOk:
            return "kOk";
        case TopicWriteStatus::kResourceExhausted:
            return "kResourceExhausted";
        case TopicWriteStatus::kFail:
            return "kFail";
        case TopicWriteStatus::kNotReady:
            return "kNotReady";
        case TopicWriteStatus::kMaxCount:
            return "kMaxCount";
    }
    UINVARIANT(false, "Unknown TopicWriteStatus value");
}

}  // namespace ydb

USERVER_NAMESPACE_END
