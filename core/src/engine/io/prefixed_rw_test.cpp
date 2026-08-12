#include <userver/utest/utest.hpp>

#include <array>
#include <chrono>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <engine/io/tests/net_listener.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/io/prefixed_rw.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/wait_any.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class MemoryRwBase final : public engine::io::RwBase {
public:
    explicit MemoryRwBase(std::string read_data)
        : read_data_(std::move(read_data))
    {}

    bool IsValid() const override { return valid_; }

    void Invalidate() { valid_ = false; }

    bool WaitReadable(engine::Deadline) override { return read_pos_ < read_data_.size(); }

    bool WaitWriteable(engine::Deadline) override { return valid_; }

    std::optional<size_t> ReadNoblock(void* buf, size_t len) override {
        if (read_pos_ >= read_data_.size()) {
            return std::nullopt;
        }
        return ReadSome(buf, len, {});
    }

    size_t ReadSome(void* buf, size_t len, engine::Deadline) override {
        if (!len || read_pos_ >= read_data_.size()) {
            return 0;
        }
        const auto to_copy = std::min(len, read_data_.size() - read_pos_);
        std::memcpy(buf, read_data_.data() + read_pos_, to_copy);
        read_pos_ += to_copy;
        return to_copy;
    }

    size_t ReadAll(void* buf, size_t len, engine::Deadline deadline) override {
        size_t done = 0;
        auto* bytes = static_cast<char*>(buf);
        while (done < len) {
            const auto n = ReadSome(bytes + done, len - done, deadline);
            if (n == 0) {
                break;
            }
            done += n;
        }
        return done;
    }

    size_t WriteAll(const void* buf, size_t len, engine::Deadline) override {
        written_.append(static_cast<const char*>(buf), len);
        return len;
    }

    const std::string& Written() const { return written_; }

private:
    std::string read_data_;
    std::size_t read_pos_{0};
    std::string written_;
    bool valid_{true};
};

}  // namespace

UTEST(PrefixedRw, ReadsPrefixThenUnderlying) {
    /// [Sample PrefixedRw]
    auto underlying = std::make_unique<MemoryRwBase>("world");
    engine::io::PrefixedRw stream("hello-", std::move(underlying));

    std::array<char, 32> buf{};
    ASSERT_EQ(11, stream.ReadAll(buf.data(), 11, {}));
    EXPECT_EQ(std::string_view(buf.data(), 11), "hello-world");
    /// [Sample PrefixedRw]
}

UTEST(PrefixedRw, ReadSomeConsumesPrefixInChunks) {
    auto underlying = std::make_unique<MemoryRwBase>("XY");
    engine::io::PrefixedRw stream("AB", std::move(underlying));

    std::array<char, 8> buf{};
    EXPECT_EQ(1, stream.ReadSome(buf.data(), 1, {}));
    EXPECT_EQ('A', buf[0]);
    EXPECT_EQ(1, stream.ReadSome(buf.data(), 1, {}));
    EXPECT_EQ('B', buf[0]);
    EXPECT_EQ(2, stream.ReadSome(buf.data(), 8, {}));
    EXPECT_EQ(std::string_view(buf.data(), 2), "XY");
}

UTEST(PrefixedRw, ReadNoblockUsesPrefixFirst) {
    auto underlying = std::make_unique<MemoryRwBase>("cd");
    engine::io::PrefixedRw stream("ab", std::move(underlying));

    std::array<char, 8> buf{};
    const auto first = stream.ReadNoblock(buf.data(), 8);
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(2, *first);
    EXPECT_EQ(std::string_view(buf.data(), 2), "ab");

    const auto second = stream.ReadNoblock(buf.data(), 8);
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(2, *second);
    EXPECT_EQ(std::string_view(buf.data(), 2), "cd");

    EXPECT_FALSE(stream.ReadNoblock(buf.data(), 8).has_value());
}

UTEST(PrefixedRw, EmptyPrefixDelegatesToUnderlying) {
    auto underlying = std::make_unique<MemoryRwBase>("data");
    engine::io::PrefixedRw stream({}, std::move(underlying));

    std::array<char, 8> buf{};
    EXPECT_EQ(4, stream.ReadAll(buf.data(), 4, {}));
    EXPECT_EQ(std::string_view(buf.data(), 4), "data");
}

UTEST(PrefixedRw, WaitReadableTrueWhilePrefixRemains) {
    auto underlying = std::make_unique<MemoryRwBase>("");
    engine::io::PrefixedRw stream("x", std::move(underlying));

    EXPECT_TRUE(stream.WaitReadable(engine::Deadline::Passed()));

    std::array<char, 1> buf{};
    ASSERT_EQ(1, stream.ReadAll(buf.data(), 1, {}));
    EXPECT_FALSE(stream.WaitReadable(engine::Deadline::Passed()));
}

UTEST(PrefixedRw, WritesGoToUnderlying) {
    auto underlying_ptr = std::make_unique<MemoryRwBase>("");
    auto* underlying = underlying_ptr.get();
    engine::io::PrefixedRw stream("ignored-on-write", std::move(underlying_ptr));

    EXPECT_EQ(3, stream.WriteAll("xyz", 3, {}));
    EXPECT_EQ("xyz", underlying->Written());

    const engine::io::IoData parts[] = {{"12", 2}, {"34", 2}};
    EXPECT_EQ(4, stream.WriteAll(parts, {}));
    EXPECT_EQ("xyz1234", underlying->Written());
}

UTEST(PrefixedRw, IsValidFollowsUnderlying) {
    auto underlying_ptr = std::make_unique<MemoryRwBase>("");
    auto* underlying = underlying_ptr.get();
    engine::io::PrefixedRw stream("p", std::move(underlying_ptr));

    EXPECT_TRUE(stream.IsValid());
    underlying->Invalidate();
    EXPECT_FALSE(stream.IsValid());
}

// Prefix makes WaitAny ready even when the peer socket has sent nothing yet.
UTEST(PrefixedRw, WaitAnyReadyWithPrefixAndEmptySocket) {
    const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
    engine::io::tests::TcpListener listener;
    auto sockets = listener.MakeSocketPair(deadline);

    // Peer has not written anything - bare socket WaitAny must time out.
    EXPECT_EQ(engine::WaitAnyFor(std::chrono::milliseconds{50}, sockets.second.GetReadableBase()), std::nullopt);

    auto underlying = std::make_unique<engine::io::Socket>(std::move(sockets.second));
    engine::io::PrefixedRw stream("preamble", std::move(underlying));

    const auto idx = engine::WaitAnyFor(std::chrono::milliseconds{50}, stream.GetReadableBase());
    ASSERT_EQ(idx, 0) << "WaitAny must wake on unread PrefixedRw prefix without socket data";

    std::array<char, 16> buf{};
    ASSERT_EQ(8, stream.ReadAll(buf.data(), 8, deadline));
    EXPECT_EQ(std::string_view(buf.data(), 8), "preamble");

    // Prefix exhausted and socket still empty - WaitAny should time out again.
    EXPECT_EQ(engine::WaitAnyFor(std::chrono::milliseconds{50}, stream.GetReadableBase()), std::nullopt);

    constexpr char kPayload[] = {'Z'};
    ASSERT_EQ(1, sockets.first.WriteAll(kPayload, sizeof(kPayload), deadline));
    ASSERT_EQ(engine::WaitAnyFor(utest::kMaxTestWaitTime, stream.GetReadableBase()), 0);
    ASSERT_EQ(1, stream.ReadAll(buf.data(), 1, deadline));
    EXPECT_EQ(buf[0], 'Z');
}

USERVER_NAMESPACE_END
