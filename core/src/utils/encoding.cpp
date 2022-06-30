#include <utils/encoding.hpp>

#include <cerrno>
#include <type_traits>
#include <vector>

#include <iconv.h>

#include <boost/lockfree/stack.hpp>

#include <userver/logging/log.hpp>
#include <utils/strerror.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::encoding {
namespace {

const auto kIconvEmpty = reinterpret_cast<iconv_t>(-1);
constexpr auto kIconvError = static_cast<size_t>(-1);

struct IconvDeleter {
  void operator()(iconv_t iconv) const noexcept { ::iconv_close(iconv); }
};
using IconvHandle =
    std::unique_ptr<std::remove_pointer_t<iconv_t>, IconvDeleter>;

[[nodiscard]] bool Reset(IconvHandle& handle) {
  const auto res = ::iconv(handle.get(), nullptr, nullptr, nullptr, nullptr);
  if (res == kIconvError) {
    const auto old_errno = errno;
    LOG_ERROR() << "error in iconv state reset (" << old_errno
                << "): " << utils::strerror(old_errno);
    return false;
  }
  return true;
}

}  // namespace

class Converter::Impl {
 public:
  Impl(std::string&& enc_from, std::string&& enc_to)
      : enc_from_(std::move(enc_from)), enc_to_(std::move(enc_to)) {}

  ~Impl() {
    iconv_t cd = kIconvEmpty;
    while (pool_.pop(cd)) {
      IconvDeleter{}(cd);
    }
  }

  IconvHandle Pop() const {
    iconv_t cd = kIconvEmpty;
    if (pool_.pop(cd)) return IconvHandle{cd};

    errno = 0;
    cd = ::iconv_open(enc_to_.c_str(), enc_from_.c_str());
    if (cd == kIconvEmpty) {
      const auto old_errno = errno;
      LOG_ERROR() << "error in iconv_open (" << old_errno
                  << "): " << utils::strerror(old_errno);
      throw std::runtime_error(std::string("iconv_open: ") +
                               utils::strerror(old_errno));
    }

    return IconvHandle{cd};
  }

  void Push(IconvHandle&& handle) const {
    if (pool_.bounded_push(handle.get())) {
      [[maybe_unused]] auto* ptr = handle.release();
    }
  }

 private:
  const std::string enc_from_;
  const std::string enc_to_;

  static const size_t kMaxIconvBufferCount = 8;
  using IconvStack =
      boost::lockfree::stack<iconv_t,
                             boost::lockfree::capacity<kMaxIconvBufferCount>>;
  mutable IconvStack pool_;
};

Converter::Converter(std::string enc_from, std::string enc_to)
    : impl_(std::move(enc_from), std::move(enc_to)) {}

Converter::~Converter() = default;

bool Converter::Convert(const char* data, size_t size,
                        std::vector<char>& out) const {
  static constexpr size_t kCoef = 2;
  static constexpr size_t kCoefLimit = 16;

  auto iconv_handle = impl_->Pop();
  for (size_t out_buf_size = size; out_buf_size <= size * kCoefLimit;
       out_buf_size *= kCoef) {
    if (!Reset(iconv_handle)) return false;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    char* pin = const_cast<char*>(data);  // it really will not change
    size_t in_size = size;

    out.resize(out_buf_size);
    char* pout = out.data();
    size_t out_size = out.size();

    errno = 0;
    const size_t res =
        ::iconv(iconv_handle.get(), &pin, &in_size, &pout, &out_size);
    if (res == kIconvError) {
      const auto old_errno = errno;
      if (old_errno != E2BIG) {
        LOG_ERROR() << "error in iconv (" << old_errno
                    << "): " << utils::strerror(old_errno);
        return false;
      }
    } else {
      out.resize(out.size() - out_size);
      impl_->Push(std::move(iconv_handle));
      return true;
    }
  }

  LOG_ERROR() << "an output buffer size is too big: " << (size * kCoefLimit);
  impl_->Push(std::move(iconv_handle));
  return false;
}

bool Converter::Convert(const std::string& in, std::string& out) const {
  std::vector<char> out_vec;
  if (!Convert(in.data(), in.size(), out_vec)) return false;
  out.assign(out_vec.data(), out_vec.size());
  return true;
}

bool Converter::Convert(const std::string& in, std::vector<char>& out) const {
  return Convert(in.data(), in.size(), out);
}

}  // namespace utils::encoding

USERVER_NAMESPACE_END
