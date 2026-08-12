#pragma once

#include <atomic>
#include <userver/engine/single_consumer_event.hpp>

#include <string_view>

#include <userver/ydb/topic_writer_types.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

/**
   Structure is used for providing information about message handling by YDB
   result is not valid until event is not ready

   @warning not used at the moment, because the functionality is not ready
*/
struct TopicWriterHandlingResult {
    std::atomic<ydb::TopicWriteEventState> result{static_cast<ydb::TopicWriteEventState>(-1)};
    engine::SingleConsumerEvent event;
};

using TopicWriterHandlingResultPtr = std::shared_ptr<TopicWriterHandlingResult>;

enum class TopicWriterWorkerState : std::size_t {
    kInitial,       // startup before first ContinuationToken receiving
    kWorking,       // after first ContinuationToken receiving
    kDisconnected,  // SessionClosedHandler event received
    kCloseInProgress,
    kDoNotUse
};

struct TopicWriterMessage {
    std::string message;
    TopicWriterHandlingResultPtr context;
};

std::string_view ToStringView(TopicWriterWorkerState state);

}  // namespace ydb::impl

USERVER_NAMESPACE_END
