#pragma once

#include <string>
#include <vector>

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::encoding {

class Converter {
 public:
  Converter(std::string enc_from, std::string enc_to);
  ~Converter();

  Converter(const Converter&) = delete;
  Converter(Converter&& other) = delete;
  Converter& operator=(const Converter&) = delete;
  Converter& operator=(Converter&& other) = delete;

  bool Convert(const char* data, size_t size, std::vector<char>& out) const;
  bool Convert(const std::string& in, std::string& out) const;
  bool Convert(const std::string& in, std::vector<char>& out) const;

 private:
  class Impl;
  static constexpr size_t kSize = 384;
  static constexpr size_t kAlignment = 64;
  utils::FastPimpl<Impl, kSize, kAlignment> impl_;
};

}  // namespace utils::encoding

USERVER_NAMESPACE_END
