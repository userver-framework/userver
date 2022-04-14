#include <userver/logging/log_extra.hpp>

#include <stdexcept>
#include <unordered_set>

#include <boost/container/small_vector.hpp>

#include <boost/stacktrace.hpp>
#include <userver/logging/level.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/consteval_map.hpp>

#include <fmt/format.h>
#include <logging/log_extra_stacktrace.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace {

LogExtra GetStacktrace(utils::Flags<impl::LogExtraStacktraceFlags> flags) {
  LogExtra ret;
  if (impl::ShouldLogStacktrace()) {
    impl::ExtendLogExtraWithStacktrace(ret, flags);
  }
  return ret;
}

}  // namespace

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-use-equals-default,hicpp-member-init,modernize-use-equals-default)
LogExtra::LogExtra() noexcept  // constructor of small_vector does not throw
{}

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
LogExtra::LogExtra(const LogExtra&) = default;

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,performance-noexcept-move-constructor,hicpp-member-init)
LogExtra::LogExtra(LogExtra&&) = default;

LogExtra::~LogExtra() = default;

// NOLINTNEXTLINE(performance-noexcept-move-constructor)
LogExtra& LogExtra::operator=(LogExtra&&) = default;

LogExtra& LogExtra::operator=(const LogExtra&) = default;

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
LogExtra::LogExtra(std::initializer_list<Pair> initial,
                   ExtendType extend_type) {
  ExtendRange(initial.begin(), initial.end(), extend_type);
}

void LogExtra::Extend(std::string key, Value value, ExtendType extend_type) {
  Extend(std::move(key),
         ProtectedValue(std::move(value), extend_type == ExtendType::kFrozen),
         extend_type);
}

void LogExtra::Extend(Pair extra, ExtendType extend_type) {
  Extend(std::move(extra.first), std::move(extra.second), extend_type);
}

void LogExtra::Extend(std::initializer_list<Pair> extra,
                      ExtendType extend_type) {
  ExtendRange(extra.begin(), extra.end(), extend_type);
}

void LogExtra::Extend(const LogExtra& extra) {
  if (extra_->empty())
    extra_ = extra.extra_;
  else
    ExtendRange(extra.extra_->begin(), extra.extra_->end());
}

void LogExtra::Extend(LogExtra&& extra) {
  if (extra_->empty()) {
    std::swap(extra_, extra.extra_);
  } else {
    for (Map::value_type& pair : *extra.extra_)
      Extend(std::move(pair.first), std::move(pair.second));
    extra.extra_->clear();
  }
}

LogExtra LogExtra::StacktraceNocache() noexcept {
  return GetStacktrace(impl::LogExtraStacktraceFlags::kNoCache);
}

LogExtra LogExtra::Stacktrace() noexcept { return GetStacktrace({}); }

const std::pair<LogExtra::Key, LogExtra::ProtectedValue>* LogExtra::Find(
    std::string_view key) const {
  for (const auto& it : *extra_)
    if (it.first == key) return &it;
  return nullptr;
}

std::pair<LogExtra::Key, LogExtra::ProtectedValue>* LogExtra::Find(
    std::string_view key) {
  for (auto& it : *extra_)
    if (it.first == key) return &it;
  return nullptr;
}

void LogExtra::SetFrozen(const std::string& key) {
  auto it = Find(key);
  if (!it)
    throw std::runtime_error("can't set frozen for non-existing key " + key);
  it->second.SetFrozen();
}

const LogExtra::Value& LogExtra::GetValue(std::string_view key) const {
  static const LogExtra::Value kEmpty{};
  auto it = Find(key);
  if (!it) return kEmpty;
  return it->second.GetValue();
}

constexpr auto kTechnicalKeys = utils::MakeConsinitSet<std::string_view>({
    {"timestamp"},
    {"level"},
    {"module"},
    {"task_id"},
    {"thread_id"},
    {"text"},
});

void LogExtra::Extend(std::string key, ProtectedValue protected_value,
                      ExtendType extend_type) {
  UINVARIANT(kTechnicalKeys.Contains(key),
             fmt::format("\"{}\" is technical key. Overwrite attempting  will "
                         "result in incorrect logs",
                         key));
  auto it = Find(key);
  if (!it) {
    extra_->emplace_back(
        std::move(key),
        extend_type == ExtendType::kFrozen
            ? ProtectedValue(std::move(protected_value.GetValue()), true)
            : std::move(protected_value));
  } else {
    it->second =
        extend_type == ExtendType::kFrozen
            ? ProtectedValue(std::move(protected_value.GetValue()), true)
            : std::move(protected_value);
  }
}

void LogExtra::Extend(Map::value_type extra, ExtendType extend_type) {
  Extend(std::move(extra.first), std::move(extra.second), extend_type);
}

LogExtra::ProtectedValue::ProtectedValue(Value value, bool frozen)
    : value_(std::move(value)), frozen_(frozen) {}

LogExtra::ProtectedValue& LogExtra::ProtectedValue::operator=(
    const ProtectedValue& other) {
  if (this == &other) return *this;

  if (frozen_) return *this;
  value_ = other.GetValue();
  frozen_ = other.frozen_;
  // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.Assign)
  return *this;
}

LogExtra::ProtectedValue& LogExtra::ProtectedValue::operator=(
    ProtectedValue&& other) noexcept {
  if (frozen_) return *this;
  value_ = std::move(other.GetValue());
  frozen_ = other.frozen_;
  return *this;
}

void LogExtra::ProtectedValue::SetFrozen() { frozen_ = true; }

}  // namespace logging

USERVER_NAMESPACE_END
