#pragma once

#include <optional>
#include <string>

#include <userver/storages/redis/impl/base.hpp>
#include <userver/storages/redis/impl/command_options.hpp>
#include <userver/storages/redis/impl/exception.hpp>

#include <userver/storages/redis/scan_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

using Longitude = USERVER_NAMESPACE::redis::Longitude;
using Latitude = USERVER_NAMESPACE::redis::Latitude;
using BoxWidth = USERVER_NAMESPACE::redis::BoxWidth;
using BoxHeight = USERVER_NAMESPACE::redis::BoxHeight;
using CommandControl = USERVER_NAMESPACE::redis::CommandControl;
using RangeOptions = USERVER_NAMESPACE::redis::RangeOptions;
using GeoaddArg = USERVER_NAMESPACE::redis::GeoaddArg;
using GeoradiusOptions = USERVER_NAMESPACE::redis::GeoradiusOptions;
using GeosearchOptions = USERVER_NAMESPACE::redis::GeosearchOptions;
using ZaddOptions = USERVER_NAMESPACE::redis::ZaddOptions;

class ScanOptionsBase {
 public:
  ScanOptionsBase() = default;
  ScanOptionsBase(ScanOptionsBase& other) = default;
  ScanOptionsBase(const ScanOptionsBase& other) = default;

  ScanOptionsBase(ScanOptionsBase&& other) = default;

  template <typename... Args>
  ScanOptionsBase(Args&&... args) {
    (Apply(std::forward<Args>(args)), ...);
  }

  class Match {
   public:
    explicit Match(std::string value) : value_(std::move(value)) {}

    const std::string& Get() const& { return value_; }

    std::string Get() && { return std::move(value_); }

   private:
    std::string value_;
  };

  class Count {
   public:
    explicit Count(size_t value) : value_(value) {}

    size_t Get() const { return value_; }

   private:
    size_t value_;
  };

  std::optional<Match> ExtractMatch() { return std::move(pattern_); }

  std::optional<Count> ExtractCount() { return std::move(count_); }

 private:
  void Apply(Match pattern) {
    if (pattern_)
      throw USERVER_NAMESPACE::redis::InvalidArgumentException(
          "duplicate Match parameter");
    pattern_ = std::move(pattern);
  }

  void Apply(Count count) {
    if (count_)
      throw USERVER_NAMESPACE::redis::InvalidArgumentException(
          "duplicate Count parameter");
    count_ = count;
  }

  std::optional<Match> pattern_;
  std::optional<Count> count_;
};

// strong typedef
template <ScanTag scan_tag>
class ScanOptionsTmpl : public ScanOptionsBase {
  using ScanOptionsBase::ScanOptionsBase;
};

using ScanOptions = ScanOptionsTmpl<ScanTag::kScan>;
using SscanOptions = ScanOptionsTmpl<ScanTag::kSscan>;
using HscanOptions = ScanOptionsTmpl<ScanTag::kHscan>;
using ZscanOptions = ScanOptionsTmpl<ScanTag::kZscan>;

void PutArg(USERVER_NAMESPACE::redis::CmdArgs::CmdArgsArray& args_,
            std::optional<ScanOptionsBase::Match> arg);
void PutArg(USERVER_NAMESPACE::redis::CmdArgs::CmdArgsArray& args_,
            std::optional<ScanOptionsBase::Count> arg);

}  // namespace storages::redis

USERVER_NAMESPACE_END
