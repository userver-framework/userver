#include <userver/utest/utest.hpp>

#include <filesystem>

#include <userver/dump/common.hpp>
#include <userver/dump/operations_encrypted.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const dump::SecretKey kTestKey{"12345678901234567890123456789012"};

enum class LegacyPermissions {
    perms_mask = 07777,
    add_perms = 0x1000,
    remove_perms = 0x2000,
    symlink_perms = 0x4000,
    kRemoveOwnerWrite = 0200 | remove_perms,
};

}  // namespace

UTEST(DumpEncFile, LegacyPermissionOptions) {
    const auto dir = fs::blocking::TempDirectory::Create();
    const auto path = dir.GetPath() + "/file";
    auto scope_time = tracing::Span::CurrentSpan().CreateScopeTime("dump");
    dump::EncryptedOperationsFactory factory(dump::SecretKey{kTestKey}, LegacyPermissions::kRemoveOwnerWrite);

    auto writer = factory.CreateWriter(path, scope_time);
    writer->Finish();

    EXPECT_EQ(std::filesystem::status(path).permissions(), std::filesystem::perms::owner_read);
}

UTEST(DumpEncFile, Smoke) {
    const auto dir = fs::blocking::TempDirectory::Create();
    const auto path = dir.GetPath() + "/file";

    auto scope_time = tracing::Span::CurrentSpan().CreateScopeTime("dump");
    dump::EncryptedWriter w(path, kTestKey, std::filesystem::perms::owner_read, scope_time);

    w.Write(1);
    UEXPECT_NO_THROW(w.Finish());

    auto size = std::filesystem::file_size(path);
    EXPECT_EQ(size, 33);

    dump::EncryptedReader r(path, kTestKey);
    EXPECT_EQ(r.Read<int32_t>(), 1);

    UEXPECT_THROW(r.Read<int32_t>(), dump::Error);

    UEXPECT_NO_THROW(r.Finish());
}

UTEST(DumpEncFile, UnreadData) {
    const auto dir = fs::blocking::TempDirectory::Create();
    const auto path = dir.GetPath() + "/file";

    auto scope_time = tracing::Span::CurrentSpan().CreateScopeTime("dump");
    dump::EncryptedWriter w(path, kTestKey, std::filesystem::perms::owner_read, scope_time);

    w.Write(1);
    UEXPECT_NO_THROW(w.Finish());

    auto size = std::filesystem::file_size(path);
    EXPECT_EQ(size, 33);

    dump::EncryptedReader r(path, kTestKey);

    UEXPECT_THROW(r.Finish(), dump::Error);
}

UTEST(DumpEncFile, Long) {
    const auto dir = fs::blocking::TempDirectory::Create();
    const auto path = dir.GetPath() + "/file";

    auto scope_time = tracing::Span::CurrentSpan().CreateScopeTime("dump");
    dump::EncryptedWriter w(path, kTestKey, std::filesystem::perms::owner_read, scope_time);

    for (int i = 0; i < 256; i++) {
        w.Write(i);
    }
    UEXPECT_NO_THROW(w.Finish());

    auto size = std::filesystem::file_size(path);
    EXPECT_EQ(size, 416);

    dump::EncryptedReader r(path, kTestKey);
    for (int i = 0; i < 256; i++) {
        EXPECT_EQ(r.Read<int32_t>(), i);
    }

    UEXPECT_THROW(r.Read<int32_t>(), dump::Error);

    UEXPECT_NO_THROW(r.Finish());
}

UTEST(DumpEncFile, ReadBackUp) {
    const auto dir = fs::blocking::TempDirectory::Create();
    const auto path = dir.GetPath() + "/file";

    auto scope_time = tracing::Span::CurrentSpan().CreateScopeTime("dump");
    dump::EncryptedWriter w(path, kTestKey, std::filesystem::perms::owner_read, scope_time);
    dump::WriteStringViewUnsafe(w, "abcdefg");
    UEXPECT_NO_THROW(w.Finish());

    dump::EncryptedReader reader(path, kTestKey);
    EXPECT_EQ(dump::ReadUnsafeAtMost(reader, 3), "abc");

    dump::BackUpReadUnsafe(reader, 2);
    EXPECT_EQ(dump::ReadUnsafeAtMost(reader, 4), "bcde");

    dump::BackUpReadUnsafe(reader, 4);
    EXPECT_EQ(dump::ReadUnsafeAtMost(reader, 5), "bcdef");

    EXPECT_EQ(dump::ReadUnsafeAtMost(reader, 1), "g");

    dump::BackUpReadUnsafe(reader, 1);
    EXPECT_EQ(dump::ReadUnsafeAtMost(reader, 1), "g");

    UEXPECT_NO_THROW(reader.Finish());
}

USERVER_NAMESPACE_END
