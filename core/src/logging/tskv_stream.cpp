#include "tskv_stream.hpp"

namespace utils {
namespace encoding {

template <>
class EncodeTskvPutCharDefault<fmt::memory_buffer> {
 public:
  void operator()(fmt::memory_buffer& to, char ch) const { to.push_back(ch); }
};

}  // namespace encoding
}  // namespace utils

namespace logging {

std::streamsize TskvBuffer::xsputn(const char_type* s, std::streamsize n) {
  buffer_.reserve(buffer_.size() + n);
  utils::encoding::EncodeTskv(buffer_, s, s + n, mode_);
  return n;
}

TskvBuffer::int_type TskvBuffer::overflow(int_type c) {
  if (c != traits_type::eof()) {
    utils::encoding::EncodeTskv(buffer_, static_cast<char>(c), mode_);
  }
  return c;
}

}  // namespace logging
