#include "log_helper_impl.hpp"

#include <fmt/chrono.h>
#include <fmt/compile.h>
#include <fmt/format.h>

#include <userver/compiler/impl/constexpr.hpp>
#include <userver/compiler/thread_local.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/encoding/tskv.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

LogHelper::Impl::Impl(
    LoggerRef logger,
    Level level,
    LogClass log_class,
    const utils::impl::SourceLocation& location
) noexcept
    : level_(std::max(level, logger.GetLevel())),
      logger_(logger),
      formatter_(logger.MakeFormatter(level, log_class, location)) {}

void LogHelper::Impl::AddText(std::string_view text) { msg_.append(text); }

size_t LogHelper::Impl::GetTextSize() const { return msg_.size(); }

void LogHelper::Impl::AddTag(std::string_view key, const LogExtra::Value& value) { formatter_->AddTag(key, value); }

void LogHelper::Impl::AddTag(std::string_view key, std::string_view value) { formatter_->AddTag(key, value); }

void LogHelper::Impl::Finish() {
    formatter_->SetText(to_string(msg_));

    auto& log_item = formatter_->ExtractLoggerItem();
    logger_.Log(level_, log_item);
}

auto LogHelper::Impl::BufferStd::overflow(int_type c) -> int_type {
    if (c == std::streambuf::traits_type::eof()) return c;
    impl_.msg_.push_back(c);
    return c;
}

std::streamsize LogHelper::Impl::BufferStd::xsputn(const char_type* s, std::streamsize n) {
    impl_.msg_.append(std::string_view(s, n));
    return n;
}

}  // namespace logging

USERVER_NAMESPACE_END
