#pragma once

/// @file userver/formats/json/validate.hpp
/// @brief json schema validator

#include <userver/formats/json/value.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {

class Schema final {
 public:
  explicit Schema(const formats::json::Value& doc);
  ~Schema();

 private:
  struct Impl;
  static constexpr std::size_t kSize = 288;
  static constexpr std::size_t kAlignment = 8;
  utils::FastPimpl<Impl, kSize, kAlignment> pimpl_;

  friend bool Validate(const formats::json::Value&,
                       const formats::json::Schema&);
};

bool Validate(const formats::json::Value& doc,
              const formats::json::Schema& schema);

}  // namespace formats::json

USERVER_NAMESPACE_END
