#include <userver/kafka/headers.hpp>
#include <userver/kafka/impl/holders.hpp>
#include <userver/utest/utest.hpp>

#include <array>

#include <gmock/gmock-matchers.h>

USERVER_NAMESPACE_BEGIN

UTEST(HeadersTest, Iterate) {
    static constexpr std::array kHeaders{
        kafka::HeaderView{"key-1", "value-1"},
        kafka::HeaderView{"key-2", "value-2"},
        kafka::HeaderView{"key-2", "value-3"},
        kafka::HeaderView{"key-4", "value-4"},
    };

    const kafka::impl::HeadersHolder holder{kHeaders};
    const auto* handle = holder.GetHandle();

    std::size_t index{0};
    for (auto header : kafka::HeadersReader{handle}) {
        EXPECT_EQ(header.name, kHeaders[index].name);
        EXPECT_EQ(header.value, kHeaders[index].value);
        index += 1;
    }
    EXPECT_EQ(index, kHeaders.size());
    {
        const kafka::HeadersReader reader{handle};
        EXPECT_EQ(reader.size(), kHeaders.size());
    }
}

UTEST(HeadersTest, ReadHeadersEmpty) {
    const kafka::impl::HeadersHolder holder{{}};

    for (auto header : kafka::HeadersReader{holder.GetHandle()}) {
        (void)header;
        EXPECT_TRUE(false);
    }
}

UTEST(HeadersTest, ReadHeadersCollect) {
    constexpr std::array kExpectedHeaders{
        kafka::HeaderView{"key-1", "value-1"},
        kafka::HeaderView{"key-2", "value-2"},
        kafka::HeaderView{"key-2", "value-3"},
        kafka::HeaderView{"key-4", "value-4"},
    };

    std::vector<kafka::OwningHeader> headers;
    {
        const std::array kHeaders{kExpectedHeaders};
        const kafka::impl::HeadersHolder holder{kExpectedHeaders};

        const kafka::HeadersReader reader{holder.GetHandle()};
        EXPECT_EQ(reader.size(), kHeaders.size());
        headers = std::vector<kafka::OwningHeader>(reader.begin(), reader.end());
    }

    EXPECT_THAT(headers, ::testing::ElementsAreArray(kExpectedHeaders));
}

USERVER_NAMESPACE_END
