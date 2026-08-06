#include <userver/engine/io/prefixed_rw.hpp>

#include <algorithm>
#include <cstring>
#include <utility>

#include <userver/engine/impl/awaiter.hpp>
#include <userver/engine/impl/context_accessor.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::io {

class PrefixedRw::ReadAwaitable final : public engine::impl::AwaitableBase {
public:
    explicit ReadAwaitable(PrefixedRw& owner) noexcept
        : owner_(owner)
    {}

    bool IsReady() const noexcept override {
        if (owner_.HasUnreadPrefix()) {
            return true;
        }
        auto token = UnderlyingReadableToken();
        if (token.IsEmpty()) {
            return false;
        }
        return token.GetAwaitable(utils::impl::InternalTag{}).IsReady();
    }

    void TryAppendAwaiter(engine::impl::AwaiterPtr& awaiter, std::uintptr_t context) override {
        if (owner_.HasUnreadPrefix()) {
            // Already ready via prefix; WaitAny must not sleep on the socket.
            return;
        }
        auto token = UnderlyingReadableToken();
        if (token.IsEmpty()) {
            return;
        }
        token.GetAwaitable(utils::impl::InternalTag{}).TryAppendAwaiter(awaiter, context);
    }

    engine::impl::AwaiterPtr RemoveAwaiter(engine::impl::Awaiter& awaiter, std::uintptr_t context) noexcept override {
        auto token = UnderlyingReadableToken();
        if (token.IsEmpty()) {
            return {};
        }
        return token.GetAwaitable(utils::impl::InternalTag{}).RemoveAwaiter(awaiter, context);
    }

    std::exception_ptr GetErrorResult() const noexcept override {
        auto token = UnderlyingReadableToken();
        if (token.IsEmpty()) {
            return {};
        }
        return token.GetAwaitable(utils::impl::InternalTag{}).GetErrorResult();
    }

private:
    AwaitableToken UnderlyingReadableToken() const noexcept {
        return owner_.underlying_->GetReadableBase().GetAwaitableToken();
    }

    PrefixedRw& owner_;
};

PrefixedRw::PrefixedRw(std::string prefix, std::unique_ptr<RwBase> underlying)
    : prefix_(std::move(prefix)),
      underlying_(std::move(underlying)),
      read_awaitable_(*this)
{
    UASSERT(underlying_);
    SetReadableAwaitableToken(AwaitableToken{utils::impl::InternalTag{}, &*read_awaitable_});
    SetWritableAwaitableToken(underlying_->GetWritableBase().GetAwaitableToken());
}

PrefixedRw::~PrefixedRw() = default;

bool PrefixedRw::IsValid() const { return underlying_->IsValid(); }

bool PrefixedRw::WaitReadable(Deadline deadline) {
    if (HasUnreadPrefix()) {
        return true;
    }
    return underlying_->WaitReadable(deadline);
}

bool PrefixedRw::WaitWriteable(Deadline deadline) { return underlying_->WaitWriteable(deadline); }

std::optional<size_t> PrefixedRw::ReadNoblock(void* buf, size_t len) {
    if (const auto from_prefix = ReadFromPrefix(buf, len)) {
        if (*from_prefix > 0 || len == 0) {
            return from_prefix;
        }
    }
    return underlying_->ReadNoblock(buf, len);
}

size_t PrefixedRw::ReadSome(void* buf, size_t len, Deadline deadline) {
    if (const auto from_prefix = ReadFromPrefix(buf, len)) {
        if (*from_prefix > 0) {
            return *from_prefix;
        }
    }
    return underlying_->ReadSome(buf, len, deadline);
}

size_t PrefixedRw::ReadAll(void* buf, size_t len, Deadline deadline) {
    size_t done = 0;
    auto* bytes = static_cast<char*>(buf);
    if (const auto from_prefix = ReadFromPrefix(bytes, len)) {
        done = *from_prefix;
    }
    if (done < len) {
        done += underlying_->ReadAll(bytes + done, len - done, deadline);
    }
    return done;
}

size_t PrefixedRw::WriteAll(const void* buf, size_t len, Deadline deadline) {
    return underlying_->WriteAll(buf, len, deadline);
}

size_t PrefixedRw::WriteAll(std::span<const IoData> list, Deadline deadline) {
    return underlying_->WriteAll(list, deadline);
}

std::optional<size_t> PrefixedRw::ReadFromPrefix(void* buf, size_t len) {
    if (prefix_pos_ >= prefix_.size() || len == 0) {
        return 0;
    }
    const auto to_copy = std::min(len, prefix_.size() - prefix_pos_);
    std::memcpy(buf, prefix_.data() + prefix_pos_, to_copy);
    prefix_pos_ += to_copy;
    return to_copy;
}

}  // namespace engine::io

USERVER_NAMESPACE_END
