#include <userver/logging/null_logger.hpp>

#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/impl/tag_writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace {

class NullFormatter final : public impl::formatters::Base {
public:
    void AddTag(std::string_view, const LogExtra::Value&) override {}

    void AddTag(std::string_view, std::string_view) override {}

    void SetText(std::string_view) override {}

    impl::formatters::LoggerItemRef ExtractLoggerItem() override { return item_; }

private:
    static impl::formatters::LoggerItemBase item_;
};

impl::formatters::LoggerItemBase NullFormatter::item_;

class NullLogger final : public impl::TextLogger {
public:
    NullLogger() noexcept : TextLogger(Format::kRaw) { LoggerBase::SetLevel(Level::kNone); }

    void SetLevel(Level) override {}
    void Log(Level, impl::formatters::LoggerItemRef) override {}
    impl::formatters::BasePtr MakeFormatter(Level, LogClass, const utils::impl::SourceLocation&) override {
        return std::make_unique<NullFormatter>();
    }
    void Flush() override {}
};

}  // namespace

TextLoggerRef GetNullLogger() noexcept {
    static NullLogger null_logger{};
    return null_logger;
}

TextLoggerPtr MakeNullLogger() { return TextLoggerPtr(std::shared_ptr<void>{}, &logging::GetNullLogger()); }

}  // namespace logging

USERVER_NAMESPACE_END
