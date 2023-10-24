#pragma once

/// @file userver/formats/json/validate.hpp
/// @brief json schema validator for string and stream

#include <iosfwd>
#include <string_view>

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {

class Schema final {
 public:
  explicit Schema(std::string_view doc);
  explicit Schema(std::istream& is);
  ~Schema();

 private:
  struct Impl;
  static constexpr std::size_t kSize = 264;
  static constexpr std::size_t kAlignment = 8;
  utils::FastPimpl<Impl, kSize, kAlignment> pimpl_;

  friend bool Validate(std::string_view doc,
                       const formats::json::Schema& schema);
  friend bool Validate(std::istream& is, const formats::json::Schema& schema);
};

bool Validate(std::string_view doc, const formats::json::Schema& schema);
bool Validate(std::istream& is, const formats::json::Schema& schema);

}  // namespace formats::json

USERVER_NAMESPACE_END
