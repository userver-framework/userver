#pragma once

#include <optional>
#include <ostream>
#include <unordered_set>

#include <fmt/format.h>

#include <userver/logging/level.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/log_extra.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl::formatters {
class Base;
using BasePtr = std::unique_ptr<Base>;
}  // namespace logging::impl::formatters

namespace logging {

inline constexpr std::size_t kInitialLogBufferSize = 1500;
using LogBuffer = fmt::basic_memory_buffer<char, kInitialLogBufferSize>;

struct LogHelper::InternalTag final {};

class LogHelper::Impl final {
public:
    explicit Impl(
        LoggerRef logger,
        Level level,
        LogClass log_class,
        const utils::impl::SourceLocation& location
    ) noexcept;

    void AddText(std::string_view text);

    size_t GetTextSize() const;

    void AddTag(std::string_view key, const LogExtra::Value& value);

    void AddTag(std::string_view key, std::string_view value);

    void Finish();

    void MarkAsBroken() {  // TODO
    }

    LogBuffer& GetBufferForRawValuePart() noexcept { return msg_; }

    bool IsStreamInitialized() const noexcept { return !!lazy_stream_; }

    std::ostream& Stream() { return GetLazyInitedStream().ostr; }

private:
    class BufferStd final : public std::streambuf {
    public:
        using char_type = std::streambuf::char_type;
        using int_type = std::streambuf::int_type;

        explicit BufferStd(Impl& impl) : impl_{impl} {}

    private:
        int_type overflow(int_type c) override;
        std::streamsize xsputn(const char_type* s, std::streamsize n) override;

        Impl& impl_;
    };

    struct LazyInitedStream {
        BufferStd sbuf;
        std::ostream ostr;

        explicit LazyInitedStream(Impl& impl) : sbuf{impl}, ostr(&sbuf) {}
    };

    LazyInitedStream& GetLazyInitedStream() {
        if (!lazy_stream_) lazy_stream_.emplace(*this);
        return *lazy_stream_;
    }

    LogBuffer msg_;
    Level level_;
    std::optional<LazyInitedStream> lazy_stream_;
    std::optional<std::unordered_set<std::string>> debug_tag_keys_;
    LoggerRef logger_;
    impl::formatters::BasePtr formatter_;
};

}  // namespace logging

USERVER_NAMESPACE_END
