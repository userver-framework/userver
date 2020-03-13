#pragma once

#include <iconv.h>
#include <vector>

#include <boost/lockfree/stack.hpp>

#include <logging/log.hpp>

namespace utils::encoding {

const std::string kUtf8 = "UTF8";
const std::string kWindows1252 = "WINDOWS-1252";
const std::string kTranslitSuffix = "//TRANSLIT";
const std::string kWindows1252Translit = kWindows1252 + kTranslitSuffix;

class Converter {
 public:
  Converter(const std::string& enc_from, const std::string& enc_to);
  ~Converter();

  Converter(const Converter&) = delete;
  Converter(Converter&& other) = delete;
  Converter& operator=(const Converter&) = delete;
  Converter& operator=(Converter&& other) = delete;

  bool Convert(const char* data, size_t size, std::vector<char>& out,
               const logging::LogExtra& log_extra = {}) const;
  bool Convert(const std::string& in, std::string& out,
               const logging::LogExtra& log_extra = {}) const;
  bool Convert(const std::string& in, std::vector<char>& out,
               const logging::LogExtra& log_extra = {}) const;

 private:
  iconv_t Pop(const logging::LogExtra& log_extra) const;
  void Push(iconv_t cd) const;

  friend class IconvKeeper;

  const std::string enc_from_;
  const std::string enc_to_;

  static const size_t kMaxIconvBufferCount = 8;
  using IconvStack =
      boost::lockfree::stack<iconv_t,
                             boost::lockfree::capacity<kMaxIconvBufferCount>>;
  mutable IconvStack cds_;
};

}  // namespace utils::encoding
