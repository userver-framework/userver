#include <utils/encoding.hpp>

#include <vector>

#include <logging/log.hpp>
#include <utils/strerror.hpp>

namespace utils::encoding {

const auto kIconvEmpty = reinterpret_cast<iconv_t>(-1);
constexpr auto kIconvError = static_cast<size_t>(-1);

class IconvKeeper {
 public:
  IconvKeeper(const Converter& conv, const logging::LogExtra& log_extra)
      : conv_(conv), log_extra_(log_extra), cd_(conv.Pop(log_extra)) {}
  ~IconvKeeper() { conv_.Push(cd_); }

  IconvKeeper(const IconvKeeper&) = delete;
  IconvKeeper& operator=(const IconvKeeper&) = delete;

  [[nodiscard]] bool Reset() const {
    const size_t res = iconv(cd_, nullptr, nullptr, nullptr, nullptr);
    if (res == kIconvError) {
      LOG_ERROR() << "error in iconv state reset (" << errno << ")"
                  << log_extra_;
      return false;
    }
    return true;
  }

  void Drop() {
    iconv_close(cd_);
    cd_ = kIconvEmpty;
  }

  operator iconv_t() const { return cd_; }

 private:
  const Converter& conv_;
  const logging::LogExtra& log_extra_;
  iconv_t cd_;
};

Converter::Converter(const std::string& enc_from, const std::string& enc_to)
    : enc_from_(enc_from), enc_to_(enc_to) {
  Push(Pop({}));  // create first
}

Converter::~Converter() {
  iconv_t cd = kIconvEmpty;
  while (cds_.pop(cd)) iconv_close(cd);
}

bool Converter::Convert(const char* data, size_t size, std::vector<char>& out,
                        const logging::LogExtra& log_extra) const {
  IconvKeeper cd(*this, log_extra);

  static const size_t kCoef = 2;
  static const size_t kCoefLimit = 16;

  for (size_t out_buf_size = size; out_buf_size <= size * kCoefLimit;
       out_buf_size *= kCoef) {
    if (!cd.Reset()) {
      cd.Drop();
      return false;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    char* pin = const_cast<char*>(data);  // it really will not change
    size_t in_size = size;

    out.resize(out_buf_size);
    char* pout = out.data();
    size_t out_size = out.size();

    errno = 0;
    const size_t res = iconv(cd, &pin, &in_size, &pout, &out_size);
    if (res == kIconvError) {
      const auto old_errno = errno;
      if (old_errno != E2BIG) {
        LOG_ERROR() << "error in iconv (" << old_errno
                    << "): " << utils::strerror(old_errno) << log_extra;
        return false;
      }
    } else {
      out.resize(out.size() - out_size);
      return true;
    }
  }

  LOG_ERROR() << "an output buffer size is too big: " << (size * kCoefLimit)
              << log_extra;
  return false;
}

bool Converter::Convert(const std::string& in, std::string& out,
                        const logging::LogExtra& log_extra) const {
  std::vector<char> out_vec;
  if (!Convert(in.data(), in.size(), out_vec, log_extra)) return false;
  out.assign(out_vec.data(), out_vec.size());
  return true;
}

bool Converter::Convert(const std::string& in, std::vector<char>& out,
                        const logging::LogExtra& log_extra) const {
  return Convert(in.data(), in.size(), out, log_extra);
}

iconv_t Converter::Pop(const logging::LogExtra& log_extra) const {
  iconv_t cd = kIconvEmpty;
  if (cds_.pop(cd)) return cd;

  errno = 0;
  cd = iconv_open(enc_to_.c_str(), enc_from_.c_str());
  if (cd == kIconvEmpty) {
    const auto old_errno = errno;
    LOG_ERROR() << "error in iconv_open (" << old_errno
                << "): " << utils::strerror(old_errno) << log_extra;
    throw std::runtime_error(std::string("iconv_open: ") +
                             utils::strerror(old_errno));
  }

  return cd;
}

void Converter::Push(iconv_t cd) const {
  if (cd == kIconvEmpty) return;
  if (cds_.bounded_push(cd)) return;
  iconv_close(cd);
}

}  // namespace utils::encoding
