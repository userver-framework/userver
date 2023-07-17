#include <userver/logging/null_logger.hpp>

#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/impl/tag_writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace {

class NullLogger final : public impl::LoggerBase {
 public:
  NullLogger() noexcept : LoggerBase(Format::kRaw) {
    LoggerBase::SetLevel(Level::kNone);
  }

  void SetLevel(Level) override {}  // do nothing
  void Log(Level, std::string_view) override {}
  void Flush() override {}
};

}  // namespace

LoggerRef GetNullLogger() noexcept {
  static NullLogger null_logger{};
  return null_logger;
}

LoggerPtr MakeNullLogger() {
  return LoggerPtr(std::shared_ptr<void>{}, &logging::GetNullLogger());
}

}  // namespace logging

USERVER_NAMESPACE_END
