#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <userver/utest/death_tests.hpp>
#include <userver/utils/iovec_advance.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct iovec MakeIovec(std::string& data) { return {data.data(), data.size()}; }

std::string_view AsStringView(const struct iovec& iov) {
    return std::string_view{static_cast<const char*>(iov.iov_base), iov.iov_len};
}

/// Emulates a `writev` that transfers at most `max_bytes` and reports how many
/// bytes it consumed, appending everything it saw to `sink`.
std::size_t FakeTransfer(const struct iovec* list, std::size_t list_size, std::size_t max_bytes, std::string& sink) {
    std::size_t transferred = 0;
    for (std::size_t i = 0; i < list_size && transferred < max_bytes; ++i) {
        const std::size_t take = std::min(list[i].iov_len, max_bytes - transferred);
        sink.append(static_cast<const char*>(list[i].iov_base), take);
        transferred += take;
    }
    return transferred;
}

}  // namespace

TEST(IovecAdvanceSingle, Partial) {
    std::string data{"0123456789"};
    struct iovec iov = MakeIovec(data);

    utils::Advance(iov, 4);

    EXPECT_EQ(AsStringView(iov), "456789");
    EXPECT_EQ(iov.iov_len, 6);
}

TEST(IovecAdvanceSingle, Zero) {
    std::string data{"0123456789"};
    struct iovec iov = MakeIovec(data);

    utils::Advance(iov, 0);

    EXPECT_EQ(AsStringView(iov), data);
}

TEST(IovecAdvanceSingle, UpToLastByte) {
    std::string data{"0123456789"};
    struct iovec iov = MakeIovec(data);

    utils::Advance(iov, data.size() - 1);

    EXPECT_EQ(AsStringView(iov), "9");
}

// `n` must be strictly less than `iov_len`, so that the buffer is never left
// empty. The precondition is guarded by UASSERT, which is compiled out in
// release builds, so the check only fires in debug ones.
#ifndef NDEBUG
TEST(IovecAdvanceSingleDeathTest, FullLengthIsForbidden) {
    std::string data{"0123456789"};
    struct iovec iov = MakeIovec(data);

    // A substring matcher rather than a regex: the asserted expressions contain
    // characters that are not valid in a POSIX extended regular expression.
    UEXPECT_DEATH(utils::Advance(iov, data.size()), testing::HasSubstr("n < iov.iov_len"));
}
#endif

TEST(IovecAdvanceIter, Zero) {
    std::string a{"aaa"};
    std::string b{"bbb"};
    const struct iovec list[]{MakeIovec(a), MakeIovec(b)};

    utils::IovIter iter{list, std::size(list)};
    utils::Advance(iter, 0);

    EXPECT_EQ(iter.iov, list);
    EXPECT_EQ(iter.iov_size, 2);
    EXPECT_EQ(iter.iov_offset, 0);
}

TEST(IovecAdvanceIter, EmptyList) {
    utils::IovIter iter{nullptr, 0};
    utils::Advance(iter, 0);

    EXPECT_EQ(iter.iov_size, 0);
    EXPECT_EQ(iter.iov_offset, 0);
}

TEST(IovecAdvanceIter, InsideFirstEntry) {
    std::string a{"aaa"};
    std::string b{"bbb"};
    const struct iovec list[]{MakeIovec(a), MakeIovec(b)};

    utils::IovIter iter{list, std::size(list)};
    utils::Advance(iter, 1);

    EXPECT_EQ(iter.iov, list);
    EXPECT_EQ(iter.iov_size, 2);
    EXPECT_EQ(iter.iov_offset, 1);
}

TEST(IovecAdvanceIter, ExactlyFirstEntry) {
    std::string a{"aaa"};
    std::string b{"bbb"};
    const struct iovec list[]{MakeIovec(a), MakeIovec(b)};

    utils::IovIter iter{list, std::size(list)};
    utils::Advance(iter, a.size());

    EXPECT_EQ(iter.iov, list + 1);
    EXPECT_EQ(iter.iov_size, 1);
    EXPECT_EQ(iter.iov_offset, 0) << "an entry consumed exactly must not leave an offset";
}

TEST(IovecAdvanceIter, AcrossEntries) {
    std::string a{"aaa"};
    std::string b{"bbb"};
    std::string c{"ccc"};
    const struct iovec list[]{MakeIovec(a), MakeIovec(b), MakeIovec(c)};

    utils::IovIter iter{list, std::size(list)};
    utils::Advance(iter, 4);

    EXPECT_EQ(iter.iov, list + 1);
    EXPECT_EQ(iter.iov_size, 2);
    EXPECT_EQ(iter.iov_offset, 1);
}

TEST(IovecAdvanceIter, EverythingConsumed) {
    std::string a{"aaa"};
    std::string b{"bbb"};
    const struct iovec list[]{MakeIovec(a), MakeIovec(b)};

    utils::IovIter iter{list, std::size(list)};
    utils::Advance(iter, a.size() + b.size());

    EXPECT_EQ(iter.iov_size, 0) << "iov points past the end and must not be dereferenced";
    EXPECT_EQ(iter.iov, std::end(list));
    EXPECT_EQ(iter.iov_offset, 0);
}

TEST(IovecAdvanceIter, SkipsEmptyEntries) {
    std::string empty;
    std::string a{"aaa"};
    const struct iovec list[]{MakeIovec(empty), MakeIovec(empty), MakeIovec(a)};

    utils::IovIter iter{list, std::size(list)};
    utils::Advance(iter, 0);

    EXPECT_EQ(iter.iov, list + 2) << "zero-length entries are consumed even by a zero advance";
    EXPECT_EQ(iter.iov_size, 1);
    EXPECT_EQ(iter.iov_offset, 0);
}

// Advancing past the total size of the buffers is guarded by UASSERT as well,
// so this check is also debug-only.
#ifndef NDEBUG
TEST(IovecAdvanceIterDeathTest, OverTotalSizeIsForbidden) {
    std::string a{"aaa"};
    const struct iovec list[]{MakeIovec(a)};

    utils::IovIter iter{list, std::size(list)};
    UEXPECT_DEATH(utils::Advance(iter, a.size() + 1), testing::HasSubstr("0 < iter.iov_size || 0 == n"));
}
#endif

/// Mirrors the read-only drain loop of `Direction::PerformIoV` and
/// `fs::blocking::FileDescriptor::Write`: the list is replaced wholesale by the
/// iterator position, and a partially consumed entry is copied out before use.
TEST(IovecAdvanceDrain, ReadOnlyList) {
    std::string a{"aaaaa"};
    std::string b{"bbbbb"};
    std::string c{"ccccc"};
    const struct iovec initial[]{MakeIovec(a), MakeIovec(b), MakeIovec(c)};
    const std::string expected = a + b + c;

    for (std::size_t chunk_limit = 1; chunk_limit <= expected.size(); ++chunk_limit) {
        const struct iovec* list = initial;
        std::size_t list_size = std::size(initial);
        std::string sink;

        while (list_size != 0) {
            const std::size_t chunk_size = FakeTransfer(list, list_size, chunk_limit, sink);
            ASSERT_GT(chunk_size, 0) << "chunk_limit=" << chunk_limit;

            utils::IovIter iter{list, list_size};
            utils::Advance(iter, chunk_size);
            if (0 == iter.iov_offset) {
                list = iter.iov;
                list_size = iter.iov_size;
            } else {
                struct iovec head = *iter.iov;
                utils::Advance(head, iter.iov_offset);
                sink.append(AsStringView(head));
                list = iter.iov + 1;
                list_size = iter.iov_size - 1;
            }
        }

        EXPECT_EQ(sink, expected) << "chunk_limit=" << chunk_limit;
    }
}

/// Mirrors the mutating drain loop of `Direction::PerformIoVMutating`, which
/// advances `list` by pointer arithmetic and trims the partially consumed entry
/// in place. Reversing the operands of the pointer difference makes `list` walk
/// backwards, which resends already transferred data and reads out of bounds.
TEST(IovecAdvanceDrain, MutableList) {
    std::string a{"aaaaa"};
    std::string b{"bbbbb"};
    std::string c{"ccccc"};
    const struct iovec initial[]{MakeIovec(a), MakeIovec(b), MakeIovec(c)};
    const std::string expected = a + b + c;

    for (std::size_t chunk_limit = 1; chunk_limit <= expected.size(); ++chunk_limit) {
        std::vector<struct iovec> buffer{std::begin(initial), std::end(initial)};
        struct iovec* list = buffer.data();
        std::size_t list_size = buffer.size();
        std::string sink;

        while (list_size != 0) {
            const std::size_t chunk_size = FakeTransfer(list, list_size, chunk_limit, sink);
            ASSERT_GT(chunk_size, 0) << "chunk_limit=" << chunk_limit;

            utils::IovIter iter{list, list_size};
            utils::Advance(iter, chunk_size);
            list += (iter.iov - list);
            list_size = iter.iov_size;
            if (0 < iter.iov_offset) {
                utils::Advance(*list, iter.iov_offset);
            }

            ASSERT_GE(list, buffer.data()) << "list walked before the start of the array";
            ASSERT_LE(list + list_size, buffer.data() + buffer.size()) << "list walked past the end of the array";
        }

        EXPECT_EQ(sink, expected) << "chunk_limit=" << chunk_limit;
    }
}

USERVER_NAMESPACE_END
