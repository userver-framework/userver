#include <compression/gzip.hpp>

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

USERVER_NAMESPACE_BEGIN

namespace compression::gzip {

namespace {
constexpr auto kDecompressBufferSize = 1024;
}

std::string Decompress(std::string_view compressed, size_t max_size) {
  std::string decompressed;

  // A guess for "small" data chunk
  // "-1" is required to avoid memory fragmentation
  // (stdlibc++ allocates capacity+1 bytes).
  decompressed.reserve(kDecompressBufferSize - 1);

  namespace bio = boost::iostreams;

  bio::filtering_istream stream;
  stream.push(bio::gzip_decompressor());
  stream.push(bio::array_source(compressed.data(), compressed.size()));

  while (stream && (decompressed.size() < max_size)) {
    char buf[kDecompressBufferSize];
    stream.read(buf, sizeof(buf));
    decompressed.insert(decompressed.end(), buf, buf + stream.gcount());
  }

  if (stream.bad())
    throw DecompressionError("failed to decompress gzip'ed data");
  if (stream) throw TooBigError();

  return decompressed;
}

}  // namespace compression::gzip

USERVER_NAMESPACE_END
