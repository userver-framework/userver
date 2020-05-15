#pragma once

#include <optional>
#include <string>

#include <storages/redis/impl/base.hpp>
#include <storages/redis/impl/command_options.hpp>
#include <storages/redis/impl/exception.hpp>

#include <storages/redis/scan_tag.hpp>

namespace storages {
namespace redis {

using CommandControl = ::redis::CommandControl;
using RangeOptions = ::redis::RangeOptions;
using ZaddOptions = ::redis::ZaddOptions;

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
      throw ::redis::InvalidArgumentException("duplicate Match parameter");
    pattern_ = std::move(pattern);
  }

  void Apply(Count count) {
    if (count_)
      throw ::redis::InvalidArgumentException("duplicate Count parameter");
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

void PutArg(::redis::CmdArgs::CmdArgsArray& args_,
            std::optional<ScanOptionsBase::Match> arg);
void PutArg(::redis::CmdArgs::CmdArgsArray& args_,
            std::optional<ScanOptionsBase::Count> arg);

}  // namespace redis
}  // namespace storages
