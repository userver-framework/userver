#include <storages/redis/impl/cmd_args.hpp>

#include <chrono>
#include <string>
#include <vector>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {
namespace {

std::vector<std::string> GetWireArgs(const CmdArgs& command) {
    std::vector<const char*> pointers;
    std::vector<std::size_t> sizes;
    command.begin()->FillPointerSizesStorages(pointers, sizes);

    std::vector<std::string> args;
    args.reserve(pointers.size());
    for (std::size_t i = 0; i < pointers.size(); ++i) {
        args.emplace_back(pointers[i], sizes[i]);
    }
    return args;
}

using KeyValues = std::vector<std::pair<std::string, std::string>>;

TEST(CmdArgsMsetex, PairCountOrderAndDuplicateKeys) {
    const auto args = GetWireArgs(CmdArgs{
        "msetex",
        3,
        KeyValues{{"key one", "value one"}, {"key-2", "NX"}, {"key one", "value-3"}},
        MsetexOptions::NoTtl()
    });

    EXPECT_EQ(
        args,
        (std::vector<std::string>{"msetex", "3", "key one", "value one", "key-2", "NX", "key one", "value-3"})
    );
}

TEST(CmdArgsMsetex, NxBeforeTtl) {
    const auto options = MsetexOptions::Expire(std::chrono::milliseconds{1234}).OnlyIfNoneOfKeysExist();
    const auto args = GetWireArgs(CmdArgs{"msetex", 1, KeyValues{{"key", "value"}}, options});

    EXPECT_EQ(args, (std::vector<std::string>{"msetex", "1", "key", "value", "NX", "PX", "1234"}));
}

TEST(CmdArgsMsetex, Xx) {
    const auto options = MsetexOptions::NoTtl().OnlyIfAllKeysExist();
    const auto args = GetWireArgs(CmdArgs{"msetex", 1, KeyValues{{"key", "value"}}, options});

    EXPECT_EQ(args, (std::vector<std::string>{"msetex", "1", "key", "value", "XX"}));
}

TEST(CmdArgsMsetex, Ex) {
    const MsetexOptions
        options{MsetexOptions::Exist::kSetAlways, MsetexOptions::TtlAction::kSetSeconds, std::chrono::seconds{42}};
    const auto args = GetWireArgs(CmdArgs{"msetex", 1, KeyValues{{"key", "value"}}, options});

    EXPECT_EQ(args, (std::vector<std::string>{"msetex", "1", "key", "value", "EX", "42"}));
}

TEST(CmdArgsMsetex, Px) {
    const auto args = GetWireArgs(CmdArgs{
        "msetex",
        1,
        KeyValues{{"key", "value"}},
        MsetexOptions::Expire(std::chrono::milliseconds{4242})
    });

    EXPECT_EQ(args, (std::vector<std::string>{"msetex", "1", "key", "value", "PX", "4242"}));
}

TEST(CmdArgsMsetex, Exat) {
    const MsetexOptions
        options{MsetexOptions::Exist::kSetAlways, MsetexOptions::TtlAction::kSetAtSeconds, std::chrono::seconds{42}};
    const auto args = GetWireArgs(CmdArgs{"msetex", 1, KeyValues{{"key", "value"}}, options});

    EXPECT_EQ(args, (std::vector<std::string>{"msetex", "1", "key", "value", "EXAT", "42"}));
}

TEST(CmdArgsMsetex, Pxat) {
    const auto deadline = std::chrono::system_clock::time_point{std::chrono::milliseconds{4242}};
    const auto args = GetWireArgs(CmdArgs{"msetex", 1, KeyValues{{"key", "value"}}, MsetexOptions::ExpireAt(deadline)});

    EXPECT_EQ(args, (std::vector<std::string>{"msetex", "1", "key", "value", "PXAT", "4242"}));
}

TEST(CmdArgsMsetex, KeepTtl) {
    const auto args = GetWireArgs(CmdArgs{"msetex", 1, KeyValues{{"key", "value"}}, MsetexOptions::KeepTtl()});

    EXPECT_EQ(args, (std::vector<std::string>{"msetex", "1", "key", "value", "KEEPTTL"}));
}

}  // namespace
}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
