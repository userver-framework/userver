#include <userver/logging/log_helper.hpp>

#include <pthread.h>

#include <iostream>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <vector>

#include <fmt/compile.h>

#include <boost/container/small_vector.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include <logging/log_extra_stacktrace.hpp>
#include <logging/log_helper_impl.hpp>
#include <userver/compiler/demangle.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/impl/tag_writer.hpp>
#include <userver/logging/level.hpp>
#include <userver/logging/log_extra.hpp>
#include <userver/logging/null_logger.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/encoding/hex.hpp>
#include <userver/utils/encoding/tskv.hpp>
#include <userver/utils/traceful_exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace {

template <typename T>
class ThreadLocalMemPool {
 public:
  static constexpr size_t kMaxSize = 1000;

  template <typename... Args>
  static std::unique_ptr<T> Pop(Args&&... args) {
    auto& pool = GetPool();
    if (pool.empty()) {
      return std::make_unique<T>(std::forward<Args>(args)...);
    }

    auto& raw = pool.back();
    // if ctor throws, memory remains in pool
    new (raw.get()) T(std::forward<Args>(args)...);
    // arm dtor, transfer ownership (noexcept)
    std::unique_ptr<T> obj(reinterpret_cast<T*>(raw.release()));
    // prune pool
    pool.pop_back();
    return obj;
  }

  // NOTE: Push might be called from a different thread than the one we got
  // the object from (where the Pop() has been called). Because of this
  // the object state must be completely torn down.
  static void Push(std::unique_ptr<T> obj) {
    auto& pool = GetPool();
    if (pool.size() >= kMaxSize) return;

    // disarm dtor, transfer ownership (noexcept)
    std::unique_ptr<StorageType> raw(
        reinterpret_cast<StorageType*>(obj.release()));
    // call dtor (might throw)
    reinterpret_cast<T*>(raw.get())->~T();
    // store into pool (may throw on container allocation)
    pool.push_back(std::move(raw));
  }

 private:
  using StorageType = std::aligned_storage_t<sizeof(T), alignof(T)>;

  static auto& GetPool() {
    thread_local std::vector<std::unique_ptr<StorageType>> pool_;
    return pool_;
  }
};

constexpr bool NeedsQuoteEscaping(char c) { return c == '\"' || c == '\\'; }

struct Module final {
  const utils::impl::SourceLocation& location;
};

LogHelper& operator<<(LogHelper& lh, Module value) {
  lh << value.location.GetFunctionName() << " ( "
     << value.location.GetFileName() << ':' << value.location.GetLine()
     << " ) ";
  return lh;
}

}  // namespace

LogHelper::LogHelper(LoggerRef logger, Level level,
                     const utils::impl::SourceLocation& location) noexcept
    : pimpl_(Impl::Make(logger, level)) {
  try {
    UASSERT(pimpl_->GetEncoding() == Encode::kNone);
    [[maybe_unused]] const auto initial_capacity = pimpl_->Capacity();

    // The following functions actually never throw if the assertions at the
    // bottom hold.
    auto tags_builder = GetTagWriter();
    tags_builder.PutTag("module", Module{location});
    logger.PrependCommonTags(tags_builder);

    OpenTextTag();
    pimpl_->MarkTextBegin();
    // Must not log further system info after this point

    UASSERT(pimpl_->GetEncoding() == Encode::kNone);

    UASSERT_MSG(
        !pimpl_->IsStreamInitialized(),
        "Some function from above initialized the std::ostream. That's a "
        "heavy operation that should be avoided. Add a breakpoint on Stream() "
        "function and tune the implementation.");
  } catch (...) {
    InternalLoggingError("Failed to log initial data");
  }
}

LogHelper::LogHelper(const LoggerPtr& logger, Level level,
                     const utils::impl::SourceLocation& location) noexcept
    : LogHelper(logger ? *logger : logging::GetNullLogger(), level, location) {}

LogHelper::~LogHelper() {
  DoLog();
  ThreadLocalMemPool<Impl>::Push(std::move(pimpl_));
}

constexpr size_t kSizeLimit = 10000;

bool LogHelper::IsLimitReached() const noexcept {
  return pimpl_->TextSize() >= kSizeLimit || pimpl_->IsBroken();
}

void LogHelper::DoLog() noexcept {
  try {
    GetTagWriter().PutLogExtra(pimpl_->GetLogExtra());
    if (pimpl_->IsStreamInitialized()) {
      Stream().flush();
    }

    pimpl_->LogTheMessage();
  } catch (...) {
    InternalLoggingError("Failed to flush log");
  }
}

std::unique_ptr<LogHelper::Impl> LogHelper::Impl::Make(LoggerRef logger,
                                                       Level level) {
  auto new_level = std::max(level, logger.GetLevel());
  return {ThreadLocalMemPool<Impl>::Pop(logger, new_level)};
}

void LogHelper::InternalLoggingError(std::string_view message) noexcept {
  try {
    std::cerr << "LogHelper: " << message << ". "
              << boost::current_exception_diagnostic_information() << '\n';
  } catch (...) {
    // ignore
  }
  pimpl_->MarkAsBroken();
  UASSERT_MSG(false, message);
}

void LogHelper::OpenTextTag() {
  Put(utils::encoding::kTskvPairsSeparator);
  Put("text");
  pimpl_->PutKeyValueSeparator();
}

void LogHelper::PutKeyValueSeparator() { pimpl_->PutKeyValueSeparator(); }

impl::TagWriter LogHelper::GetTagWriter() { return impl::TagWriter{*this}; }

impl::TagWriter LogHelper::GetTagWriterAfterText(InternalTag) {
  return GetTagWriter();
}

LogHelper& LogHelper::operator<<(char value) noexcept {
  const EncodingGuard guard{*this, Encode::kValue};
  try {
    Put(value);
  } catch (...) {
    InternalLoggingError("Failed to log char");
  }
  return *this;
}

LogHelper& LogHelper::operator<<(std::string_view value) noexcept {
  const EncodingGuard guard{*this, Encode::kValue};
  try {
    Put(value);
  } catch (...) {
    InternalLoggingError("Failed to log std::string_view");
  }
  return *this;
}

LogHelper& LogHelper::operator<<(float value) noexcept {
  UASSERT(pimpl_->GetEncoding() == Encode::kNone);
  try {
    PutFloatingPoint(value);
  } catch (...) {
    InternalLoggingError("Failed to log float");
  }
  return *this;
}

LogHelper& LogHelper::operator<<(double value) noexcept {
  UASSERT(pimpl_->GetEncoding() == Encode::kNone);
  try {
    PutFloatingPoint(value);
  } catch (...) {
    InternalLoggingError("Failed to log double");
  }
  return *this;
}

LogHelper& LogHelper::operator<<(long double value) noexcept {
  UASSERT(pimpl_->GetEncoding() == Encode::kNone);
  try {
    PutFloatingPoint(value);
  } catch (...) {
    InternalLoggingError("Failed to log long double");
  }
  return *this;
}

LogHelper& LogHelper::operator<<(unsigned long long value) noexcept {
  UASSERT(pimpl_->GetEncoding() == Encode::kNone);
  try {
    PutUnsigned(value);
  } catch (...) {
    InternalLoggingError("Failed to log unsigned");
  }
  return *this;
}

LogHelper& LogHelper::operator<<(long long value) noexcept {
  UASSERT(pimpl_->GetEncoding() == Encode::kNone);
  try {
    PutSigned(value);
  } catch (...) {
    InternalLoggingError("Failed to log signed");
  }
  return *this;
}

LogHelper& LogHelper::operator<<(bool value) noexcept {
  UASSERT(pimpl_->GetEncoding() == Encode::kNone);
  try {
    PutBoolean(value);
  } catch (...) {
    InternalLoggingError("Failed to log bool");
  }
  return *this;
}

LogHelper& LogHelper::operator<<(const std::exception& value) noexcept {
  const EncodingGuard guard{*this, Encode::kValue};
  try {
    PutException(value);
  } catch (...) {
    InternalLoggingError("Failed to log exception");
  }
  return *this;
}

LogHelper& LogHelper::operator<<(const LogExtra& extra) noexcept {
  try {
    pimpl_->GetLogExtra().Extend(extra);
  } catch (...) {
    InternalLoggingError("Failed to extend log with const LogExtra&");
  }
  return *this;
}

LogHelper& LogHelper::operator<<(LogExtra&& extra) noexcept {
  try {
    pimpl_->GetLogExtra().Extend(std::move(extra));
  } catch (...) {
    InternalLoggingError("Failed to extend log with LogExtra&&");
  }
  return *this;
}

// TODO: use std::to_chars in all the Put* functions.
void LogHelper::PutFloatingPoint(float value) {
  fmt::format_to(std::back_inserter(pimpl_->Message()), FMT_COMPILE("{}"),
                 value);
}
void LogHelper::PutFloatingPoint(double value) {
  fmt::format_to(std::back_inserter(pimpl_->Message()), FMT_COMPILE("{}"),
                 value);
}
void LogHelper::PutFloatingPoint(long double value) {
  fmt::format_to(std::back_inserter(pimpl_->Message()), FMT_COMPILE("{}"),
                 value);
}
void LogHelper::PutUnsigned(unsigned long long value) {
  fmt::format_to(std::back_inserter(pimpl_->Message()), FMT_COMPILE("{}"),
                 value);
}
void LogHelper::PutSigned(long long value) {
  fmt::format_to(std::back_inserter(pimpl_->Message()), FMT_COMPILE("{}"),
                 value);
}
void LogHelper::PutBoolean(bool value) {
  fmt::format_to(std::back_inserter(pimpl_->Message()), FMT_COMPILE("{}"),
                 value);
}

LogHelper& LogHelper::operator<<(Hex hex) noexcept {
  UASSERT(pimpl_->GetEncoding() == Encode::kNone);
  try {
    fmt::format_to(std::back_inserter(pimpl_->Message()),
                   FMT_COMPILE("0x{:016X}"), hex.value);
  } catch (...) {
    InternalLoggingError("Failed to extend log Hex");
  }
  return *this;
}

LogHelper& LogHelper::operator<<(HexShort hex) noexcept {
  UASSERT(pimpl_->GetEncoding() == Encode::kNone);
  try {
    fmt::format_to(std::back_inserter(pimpl_->Message()), FMT_COMPILE("{:X}"),
                   hex.value);
  } catch (...) {
    InternalLoggingError("Failed to extend log HexShort");
  }
  return *this;
}

LogHelper& LogHelper::operator<<(Quoted value) noexcept {
  const EncodingGuard guard{*this, Encode::kValue};
  try {
    PutQuoted(value.string);
  } catch (...) {
    InternalLoggingError("Failed to log quoted string");
  }
  return *this;
}

void LogHelper::Put(std::string_view value) {
  pimpl_->xsputn(value.data(), value.size());
}

void LogHelper::Put(char value) { pimpl_->Put(value); }

void LogHelper::PutException(const std::exception& ex) {
  if (!impl::ShouldLogStacktrace()) {
    Put(ex.what());
    Put(" (");
    Put(compiler::GetTypeName(typeid(ex)));
    Put(")");
    return;
  }

  const auto* traceful = dynamic_cast<const utils::TracefulExceptionBase*>(&ex);
  if (traceful) {
    const auto& message_buffer = traceful->MessageBuffer();
    Put(std::string_view(message_buffer.data(), message_buffer.size()));
    impl::ExtendLogExtraWithStacktrace(pimpl_->GetLogExtra(), traceful->Trace(),
                                       impl::LogExtraStacktraceFlags::kFrozen);
  } else {
    Put(ex.what());
  }
  Put(" (");
  Put(compiler::GetTypeName(typeid(ex)));
  Put(")");
}

void LogHelper::PutQuoted(std::string_view value) {
  constexpr size_t kQuotesSize = 2;

  const auto old_message_size = pimpl_->TextSize();
  const auto allowed_size =
      std::max(kSizeLimit, old_message_size + kQuotesSize) -
      (old_message_size + kQuotesSize);
  size_t used_size = 0;

  Put('\"');

  for (const char c : value) {
    const bool needs_escaping = NeedsQuoteEscaping(c);
    const size_t escaped_size = needs_escaping ? 2 : 1;

    used_size += escaped_size;
    if (used_size > allowed_size) break;

    if (needs_escaping) {
      Put('\\');
    }
    Put(c);
  }

  if (used_size > allowed_size) {
    Put("...");
  }

  Put('\"');
}

std::ostream& LogHelper::Stream() { return pimpl_->Stream(); }

LogHelper::EncodingGuard::EncodingGuard(LogHelper& lh, Encode mode) noexcept
    : lh{lh} {
  UASSERT_MSG(lh.pimpl_->GetEncoding() == Encode::kNone,
              "~EncodingGuard() sets encoding to kNone, we are expecting to "
              "have that encoding before setting the new one in guard");
  UASSERT_MSG(mode != Encode::kNone, "Already in kNone mode");

  lh.pimpl_->SetEncoding(mode);
}

LogHelper::EncodingGuard::~EncodingGuard() {
  lh.pimpl_->SetEncoding(Encode::kNone);
}

LogHelper& operator<<(LogHelper& lh, std::chrono::system_clock::time_point tp) {
  lh << utils::datetime::Timestring(tp);
  return lh;
}

LogHelper& operator<<(LogHelper& lh, std::chrono::seconds value) {
  return lh << value.count() << "s";
}

LogHelper& operator<<(LogHelper& lh, std::chrono::milliseconds value) {
  return lh << value.count() << "ms";
}

LogHelper& operator<<(LogHelper& lh, std::chrono::microseconds value) {
  return lh << value.count() << "us";
}

LogHelper& operator<<(LogHelper& lh, std::chrono::nanoseconds value) {
  return lh << value.count() << "ns";
}

LogHelper& operator<<(LogHelper& lh, std::chrono::minutes value) {
  return lh << value.count() << "min";
}

LogHelper& operator<<(LogHelper& lh, std::chrono::hours value) {
  return lh << value.count() << "h";
}

}  // namespace logging

USERVER_NAMESPACE_END
