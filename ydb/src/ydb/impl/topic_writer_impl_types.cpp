#include "topic_writer_impl_types.hpp"

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

std::string_view ToStringView(TopicWriterWorkerState state) {
    switch (state) {
        case TopicWriterWorkerState::kInitial:
            return "kInitial";
        case TopicWriterWorkerState::kWorking:
            return "kWorking";
        case TopicWriterWorkerState::kDisconnected:
            return "kDisconnected";
        case TopicWriterWorkerState::kCloseInProgress:
            return "kCloseInProgress";
        case TopicWriterWorkerState::kDoNotUse:
            UINVARIANT(false, "TopicWriterWorkerStateToString: do not used section");
            return "kDoNotUse";
    }
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
