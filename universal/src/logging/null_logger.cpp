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

    impl::formatters::LoggerItemRef ExtractLoggerItem() override { return item; }

private:
    static inline impl::formatters::LoggerItemBase item;
};

class NullLogger final : public impl::TextLogger {
public:
    NullLogger() noexcept : TextLogger(Format::kRaw) {
        LoggerBase::SetLevel(Level::kNone);
    }

    void SetLevel(Level) override {}
    void Log(Level, impl::formatters::LoggerItemRef) override {}
    impl::formatters::BasePtr MakeFormatter(Level, LogClass, const utils::impl::SourceLocation&) override {
        return std::make_unique<NullFormatter>();
    }
    void Flush() override {}
};

class NoopLogger final : public impl::TextLogger {
public:
    NoopLogger() noexcept : TextLogger(Format::kRaw) {
        SetLevel(Level::kInfo);
    }
    void Log(Level, impl::formatters::LoggerItemRef) override {}
    void Flush() override {}
};

}  // namespace

TextLoggerRef GetNullLogger() noexcept {
    static NullLogger null_logger{};
    return null_logger;
}

TextLoggerPtr MakeNullLogger() { return TextLoggerPtr(std::shared_ptr<void>{}, &logging::GetNullLogger()); }

namespace impl {

TextLoggerPtr MakeNoopLoggerForTests() { return std::make_shared<NoopLogger>(); }

}  // namespace impl

}  // namespace logging

USERVER_NAMESPACE_END
