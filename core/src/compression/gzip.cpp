#include <userver/compression/gzip.hpp>

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

USERVER_NAMESPACE_BEGIN

namespace compression::gzip {

namespace {
namespace bio = boost::iostreams;
constexpr auto kDecompressBufferSize = 1024;
}  // namespace

std::string Decompress(std::string_view compressed, size_t max_size) {
    std::string decompressed;

    // A guess for "small" data chunk
    // "-1" is required to avoid memory fragmentation
    // (stdlibc++ allocates capacity+1 bytes).
    decompressed.reserve(kDecompressBufferSize - 1);

    bio::filtering_istream stream;
    stream.push(bio::gzip_decompressor());
    stream.push(bio::array_source(compressed.data(), compressed.size()));

    while (stream) {
        char buf[kDecompressBufferSize];
        stream.read(buf, sizeof(buf));

        if (decompressed.size() + stream.gcount() > max_size) {
            throw TooBigError();
        }

        decompressed.insert(decompressed.end(), buf, buf + stream.gcount());
    }

    if (stream.bad()) {
        throw DecompressionError("failed to decompress gzip'ed data");
    }

    return decompressed;
}

std::string Compress(std::string_view data) {
    std::string compressed;
    bio::filtering_ostream stream;
    stream.push(bio::gzip_compressor{});
    stream.push(bio::back_inserter(compressed));
    stream.write(data.data(), static_cast<std::streamsize>(data.size()));
    bio::close(stream);  // flush the gzip footer
    return compressed;
}

}  // namespace compression::gzip

USERVER_NAMESPACE_END
