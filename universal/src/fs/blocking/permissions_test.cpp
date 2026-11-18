#include <userver/fs/blocking/impl/permissions.hpp>

#include <filesystem>

#include <gtest/gtest.h>
#include <boost/filesystem/operations.hpp>

#include <userver/fs/blocking/c_file.hpp>
#include <userver/fs/blocking/file_descriptor.hpp>
#include <userver/fs/blocking/open_mode.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

namespace fs::blocking {

namespace {

using BoostPerms = boost::filesystem::perms;
using StdPerms = std::filesystem::perms;
using PermOptions = std::filesystem::perm_options;

enum class UnrelatedEnum { kValue };

static_assert(impl::LegacyPermissions<BoostPerms>);
static_assert(!impl::LegacyPermissions<StdPerms>);
static_assert(!impl::LegacyPermissions<int>);
static_assert(!impl::LegacyPermissions<UnrelatedEnum>);

static_assert(impl::ToStdPermissions(BoostPerms::no_perms) == StdPerms::none);
static_assert(impl::ToStdPermissions(BoostPerms::owner_read) == StdPerms::owner_read);
static_assert(impl::ToStdPermissions(BoostPerms::owner_write) == StdPerms::owner_write);
static_assert(impl::ToStdPermissions(BoostPerms::owner_exe) == StdPerms::owner_exec);
static_assert(impl::ToStdPermissions(BoostPerms::group_read) == StdPerms::group_read);
static_assert(impl::ToStdPermissions(BoostPerms::group_write) == StdPerms::group_write);
static_assert(impl::ToStdPermissions(BoostPerms::group_exe) == StdPerms::group_exec);
static_assert(impl::ToStdPermissions(BoostPerms::others_read) == StdPerms::others_read);
static_assert(impl::ToStdPermissions(BoostPerms::others_write) == StdPerms::others_write);
static_assert(impl::ToStdPermissions(BoostPerms::others_exe) == StdPerms::others_exec);
static_assert(impl::ToStdPermissions(BoostPerms::set_uid_on_exe) == StdPerms::set_uid);
static_assert(impl::ToStdPermissions(BoostPerms::set_gid_on_exe) == StdPerms::set_gid);
static_assert(impl::ToStdPermissions(BoostPerms::sticky_bit) == StdPerms::sticky_bit);
static_assert(impl::ToStdPermissions(BoostPerms::all_all) == StdPerms::all);
static_assert(impl::ToStdPermissions(BoostPerms::perms_mask) == StdPerms::mask);
static_assert(impl::ToStdPermissions(BoostPerms::perms_not_known) == StdPerms::mask);
static_assert(
    impl::ToStdPermissions(BoostPerms::owner_read | BoostPerms::add_perms | BoostPerms::symlink_perms) ==
    StdPerms::owner_read
);

static_assert(impl::ToStdPermOptions(BoostPerms::owner_read) == PermOptions::replace);
static_assert(impl::ToStdPermOptions(BoostPerms::owner_read | BoostPerms::add_perms) == PermOptions::add);
static_assert(impl::ToStdPermOptions(BoostPerms::owner_read | BoostPerms::remove_perms) == PermOptions::remove);
static_assert(
    impl::ToStdPermOptions(BoostPerms::owner_read | BoostPerms::symlink_perms) ==
    (PermOptions::replace | PermOptions::nofollow)
);
static_assert(
    impl::ToStdPermOptions(BoostPerms::owner_read | BoostPerms::remove_perms | BoostPerms::symlink_perms) ==
    (PermOptions::remove | PermOptions::nofollow)
);

}  // namespace

TEST(FsPermissions, LegacyCreation) {
    const auto root = TempDirectory::Create();
    const auto directory_path = (std::filesystem::path{root.GetPath()} / "legacy").string();
    CreateDirectories(directory_path, BoostPerms::owner_all);
    EXPECT_EQ(std::filesystem::status(directory_path).permissions(), StdPerms::owner_all);

    const auto descriptor_path = (std::filesystem::path{directory_path} / "descriptor").string();
    const auto descriptor =
        FileDescriptor::Open(descriptor_path, {OpenFlag::kWrite, OpenFlag::kExclusiveCreate}, BoostPerms::owner_write);
    EXPECT_TRUE(descriptor.IsOpen());
    EXPECT_EQ(std::filesystem::status(descriptor_path).permissions(), StdPerms::owner_write);

    const auto file_path = (std::filesystem::path{directory_path} / "c-file").string();
    const CFile file(file_path, {OpenFlag::kWrite, OpenFlag::kExclusiveCreate}, BoostPerms::owner_read);
    EXPECT_TRUE(file.IsOpen());
    EXPECT_EQ(std::filesystem::status(file_path).permissions(), StdPerms::owner_read);
}

TEST(FsPermissions, ChmodOptions) {
    const auto file = TempFile::Create();
    const auto& path = file.GetPath();

    Chmod(path, StdPerms::owner_read);
    EXPECT_EQ(std::filesystem::status(path).permissions(), StdPerms::owner_read);

    Chmod(path, StdPerms::owner_write, PermOptions::add);
    EXPECT_EQ(std::filesystem::status(path).permissions(), StdPerms::owner_read | StdPerms::owner_write);

    Chmod(path, StdPerms::owner_read, PermOptions::remove);
    EXPECT_EQ(std::filesystem::status(path).permissions(), StdPerms::owner_write);

    Chmod(path, BoostPerms::owner_read | BoostPerms::add_perms);
    EXPECT_EQ(std::filesystem::status(path).permissions(), StdPerms::owner_read | StdPerms::owner_write);

    Chmod(path, BoostPerms::owner_write | BoostPerms::remove_perms);
    EXPECT_EQ(std::filesystem::status(path).permissions(), StdPerms::owner_read);

    Chmod(path, BoostPerms::owner_read | BoostPerms::owner_write);
    EXPECT_EQ(std::filesystem::status(path).permissions(), StdPerms::owner_read | StdPerms::owner_write);
}

TEST(FsPermissions, AtomicRewriteOptions) {
    const auto directory = TempDirectory::Create();
    const auto path = (std::filesystem::path{directory.GetPath()} / "atomic").string();
    constexpr auto kReadWrite = StdPerms::owner_read | StdPerms::owner_write;

    RewriteFileContentsAtomically(path, "std add", StdPerms::group_read, PermOptions::add);
    EXPECT_EQ(std::filesystem::status(path).permissions(), kReadWrite | StdPerms::group_read);
    EXPECT_EQ(ReadFileContents(path), "std add");

    RewriteFileContentsAtomically(path, "legacy remove", BoostPerms::owner_write | BoostPerms::remove_perms);
    EXPECT_EQ(std::filesystem::status(path).permissions(), StdPerms::owner_read);
    EXPECT_EQ(ReadFileContents(path), "legacy remove");

    RewriteFileContentsAtomically(path, "std remove", StdPerms::owner_write, PermOptions::remove);
    EXPECT_EQ(std::filesystem::status(path).permissions(), StdPerms::owner_read);
    EXPECT_EQ(ReadFileContents(path), "std remove");

    RewriteFileContentsAtomically(path, "legacy add", BoostPerms::group_read | BoostPerms::add_perms);
    EXPECT_EQ(std::filesystem::status(path).permissions(), kReadWrite | StdPerms::group_read);
    EXPECT_EQ(ReadFileContents(path), "legacy add");
}

TEST(FsPermissionsDeathTest, RejectSymlinkPermissions) {
    const auto directory = TempDirectory::Create();
    const auto target_path = (std::filesystem::path{directory.GetPath()} / "target").string();
    const auto link_path = (std::filesystem::path{directory.GetPath()} / "link").string();
    RewriteFileContents(target_path, "contents");
    const auto permissions = StdPerms::owner_read | StdPerms::owner_write;
    Chmod(target_path, permissions);
    std::filesystem::create_symlink(target_path, link_path);

    EXPECT_UINVARIANT_FAILURE_MSG(Chmod(link_path, StdPerms::owner_read), "symlink is ambiguous");
    EXPECT_UINVARIANT_FAILURE_MSG(
        Chmod(link_path, StdPerms::owner_read, PermOptions::replace | PermOptions::nofollow),
        "symlink is ambiguous"
    );
    EXPECT_UINVARIANT_FAILURE_MSG(
        Chmod(link_path, BoostPerms::owner_write | BoostPerms::remove_perms | BoostPerms::symlink_perms),
        "symlink is ambiguous"
    );
    EXPECT_UINVARIANT_FAILURE_MSG(Chmod(link_path, BoostPerms::owner_read), "symlink is ambiguous");
    EXPECT_EQ(std::filesystem::status(target_path).permissions(), permissions);
}

}  // namespace fs::blocking

USERVER_NAMESPACE_END
