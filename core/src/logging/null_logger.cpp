#include <userver/logging/null_logger.hpp>

#include <userver/logging/impl/logger_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace {

class NullLogger final : public impl::LoggerBase {
 public:
  NullLogger() : LoggerBase(Format::kRaw) { SetLevel(Level::kNone); }

  void Log(Level, std::string_view) const override {}
  void Flush() const override {}
};

}  // namespace

LoggerPtr MakeNullLogger() { return std::make_shared<NullLogger>(); }

}  // namespace logging

USERVER_NAMESPACE_END
