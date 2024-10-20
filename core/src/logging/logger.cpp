#include <userver/logging/logger.hpp>

#include <memory>

#include <engine/task/task_context.hpp>
#include <logging/impl/buffered_file_sink.hpp>
#include <logging/impl/fd_sink.hpp>
#include <logging/impl/unix_socket_sink.hpp>
#include <logging/tp_logger.hpp>

#include <userver/logging/impl/tag_writer.hpp>
#include <userver/tracing/span.hpp>

#include "config.hpp"

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace {

LoggerPtr MakeSimpleLogger(const std::string& name, impl::SinkPtr sink, Level level, Format format) {
    auto logger = std::make_shared<impl::TpLogger>(format, name);
    logger->SetLevel(level);

    if (sink) {
        logger->AddSink(std::move(sink));
    }

    return logger;
}

impl::SinkPtr MakeStderrSink() { return std::make_unique<impl::BufferedUnownedFileSink>(stderr); }

impl::SinkPtr MakeStdoutSink() { return std::make_unique<impl::BufferedUnownedFileSink>(stdout); }

}  // namespace

LoggerPtr MakeStderrLogger(const std::string& name, Format format, Level level) {
    return MakeSimpleLogger(name, MakeStderrSink(), level, format);
}

LoggerPtr MakeStdoutLogger(const std::string& name, Format format, Level level) {
    return MakeSimpleLogger(name, MakeStdoutSink(), level, format);
}

LoggerPtr MakeFileLogger(const std::string& name, const std::string& path, Format format, Level level) {
    return MakeSimpleLogger(name, std::make_unique<impl::BufferedFileSink>(path), level, format);
}

namespace impl {

void LogRaw(LoggerBase& logger, Level level, std::string_view message) {
    std::string message_with_newline;
    message_with_newline.reserve(message.size() + 1);
    message_with_newline.append(message);
    message_with_newline.push_back('\n');

    logger.Log(level, message_with_newline);
}

}  // namespace impl

namespace impl::default_ {

bool DoShouldLog(Level level) noexcept {
    const auto* const span = tracing::Span::CurrentSpanUnchecked();
    if (span) {
        const auto local_log_level = span->GetLocalLogLevel();
        if (local_log_level && *local_log_level > level) {
            return false;
        }
    }

    return true;
}

void PrependCommonTags(TagWriter writer) {
    auto* const span = tracing::Span::CurrentSpanUnchecked();
    if (span) span->LogTo(writer);

    auto* const task = engine::current_task::GetCurrentTaskContextUnchecked();
    writer.PutTag("task_id", HexShort{task});

    auto* const thread_id = reinterpret_cast<void*>(pthread_self());
    writer.PutTag("thread_id", Hex{thread_id});
}

}  // namespace impl::default_

}  // namespace logging

USERVER_NAMESPACE_END
