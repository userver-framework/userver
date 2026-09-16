#pragma once

#include <sys/uio.h>

#include <atomic>
#include <span>

#include <logging/impl/reopen_mode.hpp>
#include <userver/logging/level.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

struct LogMessage final {
    std::string_view payload;
    Level level{logging::Level::kError};
};

class BaseSink {
public:
    using IoVec = struct ::iovec;

    BaseSink(BaseSink&&) = delete;
    BaseSink& operator=(BaseSink&&) = delete;
    virtual ~BaseSink();

    void Log(std::string_view message) {
        const IoVec vec{
            .iov_base = const_cast<char*>(message.data()),  // NOLINT(cppcoreguidelines-pro-type-const-cast)
            .iov_len = message.size(),
        };
        Write(std::span<const struct iovec>{&vec, 1});
    }

    virtual void Write(std::span<const IoVec> messages) = 0;

    virtual void Reopen(ReopenMode);

    void SetLevel(Level log_level);
    Level GetLevel() const;

protected:
    BaseSink();

private:
    std::atomic<Level> level_{Level::kTrace};
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
