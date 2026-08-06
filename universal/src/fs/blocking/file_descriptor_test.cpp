#include <gtest/gtest.h>

#include <sys/uio.h>

#include <vector>

#include <userver/fs/blocking/file_descriptor.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/blocking/write.hpp>

USERVER_NAMESPACE_BEGIN

using FileDescriptor = fs::blocking::FileDescriptor;

namespace {

std::string ReadContents(FileDescriptor& fd) {
    constexpr std::size_t kBlockSize = 4096;
    std::string result;
    std::size_t size = 0;

    while (true) {
        result.resize(size + kBlockSize);
        const auto s = fd.Read({result.data() + size, kBlockSize});
        if (s == 0) {
            break;
        }
        size += s;
    }

    result.resize(size);
    return result;
}

}  // namespace

TEST(FileDescriptor, OpenExisting) {
    const auto dir = fs::blocking::TempDirectory::Create();
    const auto path = dir.GetPath() + "/foo";

    auto fd = FileDescriptor::Open(path, {fs::blocking::OpenFlag::kWrite, fs::blocking::OpenFlag::kExclusiveCreate});
    EXPECT_NE(-1, fd.GetNative());

    EXPECT_EQ(0, fd.GetSize());

    EXPECT_NO_THROW(FileDescriptor::Open(path, fs::blocking::OpenFlag::kRead));

    EXPECT_THROW(FileDescriptor::OpenDirectory(path), std::system_error);
    EXPECT_NO_THROW(std::move(fd).Close());
}

TEST(FileDescriptor, WriteRead) {
    const auto dir = fs::blocking::TempDirectory::Create();
    const auto path = dir.GetPath() + "/foo";
    const auto* contents = "123456";

    auto fd = FileDescriptor::Open(path, {fs::blocking::OpenFlag::kWrite, fs::blocking::OpenFlag::kExclusiveCreate});
    EXPECT_NO_THROW(fd.Write(contents));
    EXPECT_NO_THROW(fd.FSync());
    std::move(fd).Close();

    auto fd2 = FileDescriptor::Open(path, fs::blocking::OpenFlag::kRead);
    EXPECT_EQ(contents, ReadContents(fd2));

    fd2.Seek(0);
    EXPECT_EQ(contents, ReadContents(fd2));

    fd2.Seek(3);
    EXPECT_EQ(contents + 3, ReadContents(fd2));
}

TEST(FileDescriptor, WriteIoVec) {
    const auto dir = fs::blocking::TempDirectory::Create();
    const auto path = dir.GetPath() + "/foo";
    const std::string_view part1 = "123";
    const std::string_view part2 = "456";

    auto fd = FileDescriptor::Open(path, {fs::blocking::OpenFlag::kWrite, fs::blocking::OpenFlag::kExclusiveCreate});

    struct iovec iov[] = {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        {.iov_base = const_cast<char*>(part1.data()), .iov_len = part1.size()},
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        {.iov_base = const_cast<char*>(part2.data()), .iov_len = part2.size()},
    };
    EXPECT_NO_THROW(fd.Write(std::span<struct iovec>{iov}));
    EXPECT_NO_THROW(fd.FSync());
    std::move(fd).Close();

    auto fd2 = FileDescriptor::Open(path, fs::blocking::OpenFlag::kRead);
    EXPECT_EQ("123456", ReadContents(fd2));
}

TEST(FileDescriptor, WriteLargeIoVec) {
    constexpr std::size_t kIovCount = IOV_MAX * 8;

    const auto dir = fs::blocking::TempDirectory::Create();
    const auto path = dir.GetPath() + "/foo";

    std::vector<char> data(kIovCount);
    for (std::size_t i = 0; i < kIovCount; ++i) {
        data[i] = static_cast<char>('a' + (i % 26));
    }

    std::vector<struct iovec> iov(kIovCount);
    for (std::size_t i = 0; i < kIovCount; ++i) {
        iov[i] = {.iov_base = &data[i], .iov_len = 1};
    }

    auto fd = FileDescriptor::Open(path, {fs::blocking::OpenFlag::kWrite, fs::blocking::OpenFlag::kExclusiveCreate});
    EXPECT_NO_THROW(fd.Write(std::span<struct iovec>{iov}));
    EXPECT_NO_THROW(fd.FSync());
    std::move(fd).Close();

    auto fd2 = FileDescriptor::Open(path, fs::blocking::OpenFlag::kRead);
    EXPECT_EQ(std::string(data.begin(), data.end()), ReadContents(fd2));
}

TEST(FileDescriptor, WriteNonTruncating) {
    using fs::blocking::OpenFlag;

    const auto dir = fs::blocking::TempDirectory::Create();
    const auto path = dir.GetPath() + "/foo";

    fs::blocking::RewriteFileContents(path, "aaa");

    // Note: no kTruncate
    auto fd = FileDescriptor::Open(path, {OpenFlag::kWrite, OpenFlag::kCreateIfNotExists});
    fd.Write("bb");

    // Leftovers of the old file remain
    EXPECT_EQ(fs::blocking::ReadFileContents(path), "bba");

    fd.Write("cc");
    EXPECT_EQ(fs::blocking::ReadFileContents(path), "bbcc");

    fd.Seek(1);
    fd.Write("dd");
    EXPECT_EQ(fs::blocking::ReadFileContents(path), "bddc");

    fd.Write("<p>🐙 <b>userver</b></p>");
    EXPECT_EQ(fs::blocking::ReadFileContents(path), "bdd<p>🐙 <b>userver</b></p>");
}

TEST(FileDescriptor, WriteTruncating) {
    using fs::blocking::OpenFlag;

    const auto dir = fs::blocking::TempDirectory::Create();
    const auto path = dir.GetPath() + "/foo";

    fs::blocking::RewriteFileContents(path, "aaa");

    // Note: kTruncate
    FileDescriptor::Open(path, {OpenFlag::kWrite, OpenFlag::kCreateIfNotExists, OpenFlag::kTruncate}).Write("bb");

    // No leftovers of the old file
    EXPECT_EQ(fs::blocking::ReadFileContents(path), "bb");
}

TEST(FileDescriptor, WriteAppend) {
    using fs::blocking::OpenFlag;

    const auto dir = fs::blocking::TempDirectory::Create();
    const auto path = dir.GetPath() + "/foo";

    fs::blocking::RewriteFileContents(path, "aaa");

    // Note: kAppend
    FileDescriptor::Open(path, {OpenFlag::kWrite, OpenFlag::kCreateIfNotExists, OpenFlag::kAppend}).Write("bb");

    EXPECT_EQ(fs::blocking::ReadFileContents(path), "aaabb");
}

USERVER_NAMESPACE_END
