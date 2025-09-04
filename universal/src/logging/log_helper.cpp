#include <userver/logging/log_helper.hpp>

#include <iostream>
#include <memory>
#include <typeinfo>

#include <fmt/compile.h>
#include <boost/container/small_vector.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include <logging/log_extra_stacktrace.hpp>
#include <logging/log_helper_impl.hpp>
#include <userver/compiler/demangle.hpp>
#include <userver/compiler/thread_local.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/impl/tag_writer.hpp>
#include <userver/logging/level.hpp>
#include <userver/logging/log_extra.hpp>
#include <userver/logging/null_logger.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/datetime_light.hpp>
#include <userver/utils/traceful_exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace {

// Not boost::static_vector, because it's not constexpr-constructible.
template <typename T, std::size_t Capacity>
class StaticVector final {
public:
    bool IsFull() const noexcept { return size_ == Capacity; }

    void PushBack(T&& value) noexcept {
        UASSERT(!IsFull());
        data_[size_++] = std::move(value);
    }

    bool IsEmpty() const noexcept { return size_ == 0; }

    T& GetBack() noexcept {
        UASSERT(!IsEmpty());
        return data_[size_ - 1];
    }

    void PopBack() noexcept {
        UASSERT(!IsEmpty());
        --size_;
    }

private:
    std::size_t size_{0};
    T data_[Capacity]{};
};

template <typename T>
class ThreadLocalMemPool {
public:
    template <typename... Args>
    static std::unique_ptr<T> Pop(Args&&... args) {
        auto pool = local_storage_pool.Use();
        if (pool->IsEmpty()) {
            return std::make_unique<T>(std::forward<Args>(args)...);
        }

        auto& raw = pool->GetBack();
        // if ctor throws, memory remains in pool
        new (raw.get()) T(std::forward<Args>(args)...);
        // arm dtor, transfer ownership (noexcept)
        std::unique_ptr<T> obj(reinterpret_cast<T*>(raw.release()));
        // prune pool
        pool->PopBack();
        return obj;
    }

    // NOTE: Push might be called from a different thread than the one we got
    // the object from (where the Pop() has been called). Because of this
    // the object state must be completely torn down.
    static void Push(std::unique_ptr<T> obj) noexcept {
        auto pool = local_storage_pool.Use();
        if (pool->IsFull()) return;

        // disarm dtor, transfer ownership (noexcept)
        std::unique_ptr<Storage> raw(reinterpret_cast<Storage*>(obj.release()));
        // call dtor
        std::destroy_at(reinterpret_cast<T*>(raw.get()));
        // store into pool
        pool->PushBack(std::move(raw));
    }

private:
    // If there is no logging and context switches within the lifetime of a
    // LogHelper (the usual case), then kMaxSize = 1 is enough.
    // Having more cache entries instead allows to retain performance
    // in some edge cases.
    static constexpr std::size_t kMaxSize = 16;

    using Storage = std::aligned_storage_t<sizeof(T), alignof(T)>;
    using StoragePool = StaticVector<std::unique_ptr<Storage>, kMaxSize>;

    static inline compiler::ThreadLocal local_storage_pool = [] { return StoragePool{}; };
};

constexpr bool NeedsQuoteEscaping(char c) { return c == '\"' || c == '\\'; }

}  // namespace

LogHelper::LogHelper(
    LoggerRef logger,
    Level level,
    LogClass log_class,
    const utils::impl::SourceLocation& location
) noexcept
    : pimpl_(ThreadLocalMemPool<Impl>::Pop(logger, level, log_class, location)) {
    try {
        logger.PrependCommonTags(GetTagWriter());
    } catch (...) {
        InternalLoggingError("Failed to log initial data");
    }
}

LogHelper::LogHelper(
    const LoggerPtr& logger,
    Level level,
    LogClass log_class,
    const utils::impl::SourceLocation& location
) noexcept
    : LogHelper(logger ? *logger : logging::GetNullLogger(), level, log_class, location) {}

LogHelper::~LogHelper() {
    DoLog();
    ThreadLocalMemPool<Impl>::Push(std::move(pimpl_));
}

constexpr size_t kSizeLimit = 10000;

bool LogHelper::IsLimitReached() const noexcept { return pimpl_->GetTextSize() >= kSizeLimit; }

void LogHelper::DoLog() noexcept {
    try {
        pimpl_->Finish();
    } catch (...) {
        InternalLoggingError("Failed to flush log");
    }
}

void LogHelper::InternalLoggingError(std::string_view message) noexcept {
    try {
        std::cerr << "LogHelper: " << message << ". " << boost::current_exception_diagnostic_information() << '\n';
    } catch (...) {
        // ignore
    }
    pimpl_->MarkAsBroken();
    UASSERT_MSG(false, message);
}

impl::TagWriter LogHelper::GetTagWriter() { return impl::TagWriter{*this}; }

LogHelper& LogHelper::operator<<(char value) noexcept {
    try {
        Put(value);
    } catch (...) {
        InternalLoggingError("Failed to log char");
    }
    return *this;
}

LogHelper& LogHelper::operator<<(std::string_view value) noexcept {
    try {
        Put(value);
    } catch (...) {
        InternalLoggingError("Failed to log std::string_view");
    }
    return *this;
}

LogHelper& LogHelper::operator<<(float value) noexcept {
    try {
        PutFloatingPoint(value);
    } catch (...) {
        InternalLoggingError("Failed to log float");
    }
    return *this;
}

LogHelper& LogHelper::operator<<(double value) noexcept {
    try {
        PutFloatingPoint(value);
    } catch (...) {
        InternalLoggingError("Failed to log double");
    }
    return *this;
}

LogHelper& LogHelper::operator<<(long double value) noexcept {
    try {
        PutFloatingPoint(value);
    } catch (...) {
        InternalLoggingError("Failed to log long double");
    }
    return *this;
}

LogHelper& LogHelper::operator<<(unsigned long long value) noexcept {
    try {
        PutUnsigned(value);
    } catch (...) {
        InternalLoggingError("Failed to log unsigned");
    }
    return *this;
}

LogHelper& LogHelper::operator<<(long long value) noexcept {
    try {
        PutSigned(value);
    } catch (...) {
        InternalLoggingError("Failed to log signed");
    }
    return *this;
}

LogHelper& LogHelper::operator<<(bool value) noexcept {
    try {
        PutBoolean(value);
    } catch (...) {
        InternalLoggingError("Failed to log bool");
    }
    return *this;
}

LogHelper& LogHelper::operator<<(const std::exception& value) noexcept {
    try {
        PutException(value);
    } catch (...) {
        InternalLoggingError("Failed to log exception");
    }
    return *this;
}

LogHelper& LogHelper::operator<<(const LogExtra& extra) noexcept {
    try {
        for (const auto& [key, value] : *extra.extra_) {
            PutTag(key, value.GetValue());
        }
    } catch (...) {
        InternalLoggingError("Failed to extend log with const LogExtra&");
    }
    return *this;
}

LogHelper& LogHelper::operator<<(LogExtra&& extra) noexcept {
    try {
        for (auto& [key, value] : *extra.extra_) {
            PutTag(key, std::move(value.GetValue()));
        }
    } catch (...) {
        InternalLoggingError("Failed to extend log with LogExtra&&");
    }
    return *this;
}

LogHelper& LogHelper::PutTag(std::string_view key, const LogExtra::Value& value) noexcept {
    try {
        pimpl_->AddTag(key, value);
    } catch (...) {
        InternalLoggingError("Failed to extend log with LogExtra&&");
    }
    return *this;
}

LogHelper& LogHelper::PutSwTag(std::string_view key, std::string_view value) noexcept {
    try {
        pimpl_->AddTag(key, value);
    } catch (...) {
        InternalLoggingError("Failed to extend log with LogExtra&&");
    }
    return *this;
}

void LogHelper::PutFloatingPoint(float value) {
    fmt::format_to(fmt::appender(pimpl_->GetBufferForRawValuePart()), FMT_COMPILE("{}"), value);
}
void LogHelper::PutFloatingPoint(double value) {
    fmt::format_to(fmt::appender(pimpl_->GetBufferForRawValuePart()), FMT_COMPILE("{}"), value);
}
void LogHelper::PutFloatingPoint(long double value) {
    fmt::format_to(fmt::appender(pimpl_->GetBufferForRawValuePart()), FMT_COMPILE("{}"), value);
}
void LogHelper::PutUnsigned(unsigned long long value) {
    fmt::format_to(fmt::appender(pimpl_->GetBufferForRawValuePart()), FMT_COMPILE("{}"), value);
}
void LogHelper::PutSigned(long long value) {
    fmt::format_to(fmt::appender(pimpl_->GetBufferForRawValuePart()), FMT_COMPILE("{}"), value);
}
void LogHelper::PutBoolean(bool value) {
    fmt::format_to(fmt::appender(pimpl_->GetBufferForRawValuePart()), FMT_COMPILE("{}"), value);
}

LogHelper& LogHelper::operator<<(Hex hex) noexcept {
    try {
        fmt::format_to(fmt::appender(pimpl_->GetBufferForRawValuePart()), FMT_COMPILE("0x{:016X}"), hex.value);
    } catch (...) {
        InternalLoggingError("Failed to extend log Hex");
    }
    return *this;
}

LogHelper& LogHelper::operator<<(HexShort hex) noexcept {
    try {
        fmt::format_to(fmt::appender(pimpl_->GetBufferForRawValuePart()), FMT_COMPILE("{:X}"), hex.value);
    } catch (...) {
        InternalLoggingError("Failed to extend log HexShort");
    }
    return *this;
}

LogHelper& LogHelper::operator<<(Quoted value) noexcept {
    try {
        PutQuoted(value.string);
    } catch (...) {
        InternalLoggingError("Failed to log quoted string");
    }
    return *this;
}

void LogHelper::Put(std::string_view value) { pimpl_->AddText(value); }

void LogHelper::Put(char value) { pimpl_->AddText(std::string_view(&value, 1)); }

void LogHelper::PutRaw(std::string_view value_needs_no_escaping) {
    pimpl_->GetBufferForRawValuePart().append(value_needs_no_escaping);
}

void LogHelper::PutException(const std::exception& ex) {
    if (!impl::ShouldLogStacktrace()) {
        Put(ex.what());
        PutRaw(" (");
        PutRaw(compiler::GetTypeName(typeid(ex)));
        PutRaw(")");
        return;
    }

    const auto* traceful = dynamic_cast<const utils::TracefulExceptionBase*>(&ex);
    if (traceful) {
        const auto& message_buffer = traceful->MessageBuffer();
        Put(std::string_view(message_buffer.data(), message_buffer.size()));
        LogExtra log_extra;
        impl::ExtendLogExtraWithStacktrace(log_extra, traceful->Trace(), impl::LogExtraStacktraceFlags::kFrozen);
        *this << log_extra;
    } else {
        Put(ex.what());
    }
    PutRaw(" (");
    PutRaw(compiler::GetTypeName(typeid(ex)));
    PutRaw(")");
}

void LogHelper::PutQuoted(std::string_view value) {
    constexpr size_t kQuotesSize = 2;

    const auto old_message_size = pimpl_->GetTextSize();
    const auto allowed_size = std::max(kSizeLimit, old_message_size + kQuotesSize) - (old_message_size + kQuotesSize);
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
        PutRaw("...");
    }

    Put('\"');
}

void LogHelper::VFormat(fmt::string_view fmt, fmt::format_args args) noexcept {
    try {
        fmt::vformat_to(fmt::appender(pimpl_->GetBufferForRawValuePart()), fmt, args);
    } catch (...) {
        InternalLoggingError("Failed to extend log with fmt::format_args");
    }
}

std::ostream& LogHelper::Stream() { return pimpl_->Stream(); }

void LogHelper::FlushStream() { pimpl_->Stream().flush(); }

LogHelper& operator<<(LogHelper& lh, std::chrono::system_clock::time_point tp) {
    lh << utils::datetime::UtcTimestring(tp);
    return lh;
}

LogHelper& operator<<(LogHelper& lh, std::chrono::seconds value) { return lh << value.count() << "s"; }

LogHelper& operator<<(LogHelper& lh, std::chrono::milliseconds value) { return lh << value.count() << "ms"; }

LogHelper& operator<<(LogHelper& lh, std::chrono::microseconds value) { return lh << value.count() << "us"; }

LogHelper& operator<<(LogHelper& lh, std::chrono::nanoseconds value) { return lh << value.count() << "ns"; }

LogHelper& operator<<(LogHelper& lh, std::chrono::minutes value) { return lh << value.count() << "min"; }

LogHelper& operator<<(LogHelper& lh, std::chrono::hours value) { return lh << value.count() << "h"; }

}  // namespace logging

USERVER_NAMESPACE_END
