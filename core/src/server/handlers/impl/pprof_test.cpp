#include <userver/server/handlers/impl/pprof.hpp>

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include <fmt/format.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace {

namespace impl = server::handlers::impl;

using Addresses = std::vector<std::uintptr_t>;

std::string PprofTestNamedFunction() { return "the name of this function is looked up in the symbol table"; }

std::string MakeBody(std::size_t addresses_count) {
    std::string body;
    for (std::size_t address = 1; address <= addresses_count; ++address) {
        if (!body.empty()) {
            body += '+';
        }
        body += fmt::format("0x{:x}", address);
    }
    return body;
}

TEST(PprofSymbolAddresses, EmptyIsRejected) {
    EXPECT_THAT(
        impl::ParsePprofSymbolAddresses(""),
        testing::VariantWith<impl::PprofSymbolParseError>(impl::PprofSymbolParseError::kInvalidAddress)
    );
}

TEST(PprofSymbolAddresses, SingleAddress) {
    EXPECT_THAT(impl::ParsePprofSymbolAddresses("0x2a"), testing::VariantWith<Addresses>(Addresses{42}));
    EXPECT_THAT(impl::ParsePprofSymbolAddresses("0X2A"), testing::VariantWith<Addresses>(Addresses{42}));
    EXPECT_THAT(impl::ParsePprofSymbolAddresses("2a"), testing::VariantWith<Addresses>(Addresses{42}));
}

TEST(PprofSymbolAddresses, OrderIsPreserved) {
    EXPECT_THAT(impl::ParsePprofSymbolAddresses("0x3+0x1+0x2"), testing::VariantWith<Addresses>(Addresses{3, 1, 2}));
}

TEST(PprofSymbolAddresses, DuplicatesAreDropped) {
    EXPECT_THAT(impl::ParsePprofSymbolAddresses("0x1+0x2+0x1+0X1+1"), testing::VariantWith<Addresses>(Addresses{1, 2}));
}

TEST(PprofSymbolAddresses, InvalidTokensAreRejected) {
    for (const std::string_view body : {"++", "0x", "0x0+0x00", "zz+0x3", "0x4zz", " 0x5", "0x6 "}) {
        EXPECT_THAT(
            impl::ParsePprofSymbolAddresses(body),
            testing::VariantWith<impl::PprofSymbolParseError>(impl::PprofSymbolParseError::kInvalidAddress)
        ) << body;
    }
}

TEST(PprofSymbolAddresses, LimitIsRespected) {
    const auto body = MakeBody(impl::kMaxPprofSymbolAddresses);

    const auto at_limit = impl::ParsePprofSymbolAddresses(body);
    ASSERT_TRUE(std::holds_alternative<Addresses>(at_limit));
    EXPECT_EQ(std::get<Addresses>(at_limit).size(), impl::kMaxPprofSymbolAddresses);

    EXPECT_THAT(
        impl::ParsePprofSymbolAddresses(body + "+0xdeadbeef"),
        testing::VariantWith<impl::PprofSymbolParseError>(impl::PprofSymbolParseError::kTooManyAddresses)
    );
    EXPECT_THAT(impl::ParsePprofSymbolAddresses(body + "+0x1"), testing::VariantWith<Addresses>(testing::_));
}

TEST(PprofSymbolAddresses, SymbolizesNamedFunction) {
    const auto* const function = reinterpret_cast<const void*>(&PprofTestNamedFunction);
    const auto function_address = reinterpret_cast<std::uintptr_t>(function);

    const auto addresses = impl::ParsePprofSymbolAddresses(fmt::format("0x{:x}", function_address));
    ASSERT_TRUE(std::holds_alternative<Addresses>(addresses));
    EXPECT_EQ(std::get<Addresses>(addresses), Addresses{function_address});

    const auto response = impl::SymbolizePprofAddresses(std::get<Addresses>(addresses));
    if (response.empty()) {
        GTEST_SKIP() << "The build provides no symbol names to resolve the address to";
    }
    EXPECT_THAT(response, testing::HasSubstr("PprofTestNamedFunction"));
}

}  // namespace

USERVER_NAMESPACE_END
