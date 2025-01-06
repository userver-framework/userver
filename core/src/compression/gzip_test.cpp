#include <gtest/gtest.h>

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <compression/gzip.hpp>

USERVER_NAMESPACE_BEGIN

TEST(Gzip, TestOverflow) {
    const std::string big_msg("This is a \"Very long\" msg!");

    namespace bio = boost::iostreams;

    constexpr auto kCompBufSize = 1024;

    bio::filtering_istream stream;
    stream.push(bio::gzip_compressor());
    stream.push(bio::array_source(big_msg.data(), big_msg.size()));

    std::string compressed;

    while (stream) {
        char buf[kCompBufSize];
        stream.read(buf, sizeof(buf));
        compressed.insert(compressed.end(), buf, buf + stream.gcount());
    }

    EXPECT_THROW(compression::gzip::Decompress(compressed, big_msg.size() / 2), compression::TooBigError);
}

USERVER_NAMESPACE_END
