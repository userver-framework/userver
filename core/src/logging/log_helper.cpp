#include <userver/logging/log_helper.hpp>

#include <pthread.h>

#include <iostream>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <vector>

#include <boost/container/small_vector.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include <engine/task/task_context.hpp>
#include <logging/log_extra_stacktrace.hpp>
#include <logging/log_helper_impl.hpp>
#include <userver/compiler/demangle.hpp>
#include <userver/logging/log_extra.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/traceful_exception.hpp>

namespace logging {

constexpr char kPathLineSeparator = ':';

// uses stringify
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NOTHROW_CALL_BASE(ERROR_PREFIX, FUNCTION)                             \
  try {                                                                       \
    FUNCTION;                                                                 \
  } catch (...) {                                                             \
    try {                                                                     \
      std::cerr << ERROR_PREFIX "failed to " #FUNCTION ":"                    \
                << boost::current_exception_diagnostic_information() << '\n'; \
      UASSERT_MSG(false, #FUNCTION);                                          \
    } catch (...) {                                                           \
      UASSERT_MSG(false, #FUNCTION " (second catch)");                        \
    }                                                                         \
  }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NOTHROW_CALL_CONSTRUCTOR(PATH, LINE, FUNCTION) \
  NOTHROW_CALL_BASE((PATH) << kPathLineSeparator << (LINE) << ": ", (FUNCTION))

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NOTHROW_CALL_GENERIC(FUNCTION) NOTHROW_CALL_BASE("LogHelper ", FUNCTION)

namespace {

template <typename T>
class ThreadLocalMemPool {
 public:
  static constexpr size_t kMaxSize = 1000;

  template <typename... Args>
  static std::unique_ptr<T> Pop(Args&&... args) {
    auto& pool = GetPool();
    if (pool.empty()) {
      // https://bugs.llvm.org/show_bug.cgi?id=38176
      // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
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

}  // namespace

LogHelper::LogHelper(LoggerPtr logger, Level level, std::string_view path,
                     int line, std::string_view func, Mode mode) noexcept
    : pimpl_(ThreadLocalMemPool<Impl>::Pop(std::move(logger), level)) {
  [[maybe_unused]] const auto initial_capacity = pimpl_->Capacity();

  // The following functions actually never throw if the assertions at the
  // bottom hold.
  if (mode != Mode::kNoSpan) {
    NOTHROW_CALL_CONSTRUCTOR(path, line, LogSpan())
  }
  NOTHROW_CALL_CONSTRUCTOR(path, line, LogModule(path, line, func))
  NOTHROW_CALL_CONSTRUCTOR(path, line, LogIds())

  LogTextKey();
  pimpl_->MarkTextBegin();
  // Must not log further system info after this point

  UASSERT_MSG(
      !pimpl_->IsStreamInitialized(),
      "Some function frome above initialized the std::ostream. That's a "
      "heavy operation that should be avoided. Add a breakpoint on Stream() "
      "function and tune the implementation.");

  UASSERT_MSG(initial_capacity == pimpl_->Capacity(),
              "Logging buffer is too small to keep initial data. Adjust the "
              "pimpl_ or reduce the output of the above functions.");
}

LogHelper::~LogHelper() {
  DoLog();
  ThreadLocalMemPool<Impl>::Push(std::move(pimpl_));
}

constexpr size_t kSizeLimit = 10000;

bool LogHelper::IsLimitReached() const {
  return pimpl_->TextSize() >= kSizeLimit;
}

void LogHelper::DoLog() noexcept {
  NOTHROW_CALL_GENERIC(AppendLogExtra())
  if (pimpl_->IsStreamInitialized()) {
    NOTHROW_CALL_GENERIC(Stream().flush());
  }

  try {
    pimpl_->LogTheMessage();
  } catch (...) {
    try {
      std::cerr << "LogHelper failed to log the message:"
                << boost::current_exception_diagnostic_information() << '\n';

      NOTHROW_CALL_GENERIC(std::cerr << pimpl_->StreamBuf());
    } catch (...) {
    }
    UASSERT_MSG(false, "LogHelper::DoLog()");
  }
}

void LogHelper::AppendLogExtra() {
  const auto& items = pimpl_->GetLogExtra().extra_;
  if (items->empty()) return;

  for (const auto& item : *items) {
    Put(utils::encoding::kTskvPairsSeparator);
    {
      EncodingGuard guard{*this, Encode::kKeyReplacePeriod};
      Put(item.first);
    }
    Put(utils::encoding::kTskvKeyValueSeparator);
    std::visit([this](const auto& value) { *this << value; },
               item.second.GetValue());
  }
}

void LogHelper::LogTextKey() {
  Put(utils::encoding::kTskvPairsSeparator);
  Put("text");
  Put(utils::encoding::kTskvKeyValueSeparator);
}

void LogHelper::LogModule(std::string_view path, int line,
                          std::string_view func) {
  Put("module");
  Put(utils::encoding::kTskvKeyValueSeparator);
  Put(func);
  Put(" ( ");
  Put(path);
  Put(kPathLineSeparator);
  PutSigned(line);
  Put(" ) ");
}

void LogHelper::LogIds() {
  auto task = engine::current_task::GetCurrentTaskContextUnchecked();
  uint64_t task_id = task ? reinterpret_cast<uint64_t>(task) : 0;
  auto thread_id = reinterpret_cast<void*>(pthread_self());

  Put(utils::encoding::kTskvPairsSeparator);
  Put("task_id");
  Put(utils::encoding::kTskvKeyValueSeparator);
  *this << HexShort{task_id};

  Put(utils::encoding::kTskvPairsSeparator);
  Put("thread_id");
  Put(utils::encoding::kTskvKeyValueSeparator);
  *this << Hex{thread_id};
}

void LogHelper::LogSpan() {
  auto* span = tracing::Span::CurrentSpanUnchecked();
  if (span) *this << *span;
}

LogHelper& LogHelper::operator<<(const LogExtra& extra) {
  pimpl_->GetLogExtra().Extend(extra);
  return *this;
}

LogHelper& LogHelper::operator<<(LogExtra&& extra) {
  pimpl_->GetLogExtra().Extend(std::move(extra));
  return *this;
}

// TODO: use std::to_chars in all the Put* functions.
void LogHelper::PutFloatingPoint(float value) {
  format_to(pimpl_->Message(), "{}", value);
}
void LogHelper::PutFloatingPoint(double value) {
  format_to(pimpl_->Message(), "{}", value);
}
void LogHelper::PutFloatingPoint(long double value) {
  format_to(pimpl_->Message(), "{}", value);
}
void LogHelper::PutUnsigned(unsigned long long value) {
  format_to(pimpl_->Message(), "{}", value);
}
void LogHelper::PutSigned(long long value) {
  format_to(pimpl_->Message(), "{}", value);
}
void LogHelper::PutBoolean(bool value) {
  format_to(pimpl_->Message(), "{}", value);
}

LogHelper& LogHelper::operator<<(Hex hex) {
  format_to(pimpl_->Message(), "0x{:016X}", hex.value);
  return *this;
}

LogHelper& LogHelper::operator<<(HexShort hex) {
  format_to(pimpl_->Message(), "{:X}", hex.value);
  return *this;
}

void LogHelper::Put(std::string_view value) {
  pimpl_->xsputn(value.data(), value.size());
}

void LogHelper::Put(char value) { pimpl_->xsputn(&value, 1); }

void LogHelper::PutException(const std::exception& ex) {
  if (!impl::ShouldLogStacktrace()) {
    Put(ex.what());
    *this << " (" << compiler::GetTypeName(typeid(ex)) << ")";
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
  *this << " (" << compiler::GetTypeName(typeid(ex)) << ")";
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
