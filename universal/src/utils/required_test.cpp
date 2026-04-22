#include <userver/utils/required.hpp>

#include <concepts>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace {

struct NonDefaulted final {
    explicit NonDefaulted(int value)
        : value(value)
    {}

    int value;
};

struct MultiArg final {
    MultiArg(int a, std::string b)
        : a(a),
          b(std::move(b))
    {}

    int a;
    std::string b;
};

}  // namespace

/// [sample]
struct MyConfig {
    utils::Required<std::string> name;
    utils::Required<int> timeout;
};
/// [sample]

TEST(UtilsRequired, Sample) {
    /// [sample usage]
    MyConfig cfg{.name = "my-service", .timeout = 42};
    EXPECT_EQ(*cfg.name, "my-service");
    EXPECT_EQ(*cfg.timeout, 42);
    /// [sample usage]
}

TEST(UtilsRequired, ConstructionConcepts) {
    static_assert(!std::default_initializable<utils::Required<int>>);
    static_assert(!std::default_initializable<utils::Required<std::string>>);
    static_assert(!std::default_initializable<utils::Required<NonDefaulted>>);

    static_assert(std::constructible_from<utils::Required<std::string>, const char*>);
    static_assert(std::constructible_from<utils::Required<std::string>, std::string_view>);
    static_assert(std::constructible_from<utils::Required<std::string>, std::size_t, char>);

    static_assert(std::constructible_from<utils::Required<NonDefaulted>, int>);
    static_assert(!std::constructible_from<utils::Required<NonDefaulted>, std::string>);

    static_assert(std::constructible_from<utils::Required<MultiArg>, int, std::string>);
}

TEST(UtilsRequired, Implicit) {
    // const char* is implicitly convertible to std::string
    static_assert(std::convertible_to<const char*, utils::Required<std::string>>);
    // string_view is explicitly constructible but not implicitly convertible to std::string
    static_assert(!std::convertible_to<std::string_view, utils::Required<std::string>>);
    // int is not implicitly convertible to NonDefaulted (explicit ctor)
    static_assert(!std::convertible_to<int, utils::Required<NonDefaulted>>);
}

TEST(UtilsRequired, Deref) {
    utils::Required<std::string> r{"hello"};
    EXPECT_EQ(*r, "hello");
    EXPECT_EQ(r->size(), 5u);
}

TEST(UtilsRequired, DerefConst) {
    const utils::Required<std::string> r{"world"};
    EXPECT_EQ(*r, "world");
    EXPECT_EQ(r->size(), 5u);
}

TEST(UtilsRequired, DerefMove) {
    utils::Required<std::string> r{"move-me"};
    const std::string moved = *std::move(r);
    EXPECT_EQ(moved, "move-me");
}

TEST(UtilsRequired, Mutate) {
    utils::Required<std::string> r{"initial"};
    *r = "modified";
    EXPECT_EQ(*r, "modified");
}

TEST(UtilsRequired, CopyAndMove) {
    utils::Required<std::string> a{"original"};
    utils::Required<std::string> b = a;
    EXPECT_EQ(*b, "original");

    utils::Required<std::string> c = std::move(a);
    EXPECT_EQ(*c, "original");
}

TEST(UtilsRequired, EmplaceMultiArg) {
    utils::Required<MultiArg> r{42, std::string{"hello"}};
    EXPECT_EQ(r->a, 42);
    EXPECT_EQ(r->b, "hello");
}

TEST(UtilsRequired, ExplicitSingleArg) {
    utils::Required<NonDefaulted> r{7};
    EXPECT_EQ(r->value, 7);
}

USERVER_NAMESPACE_END
