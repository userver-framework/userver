#include "log_extra.hpp"

#include <stdexcept>

#include "log.hpp"

namespace logging {

LogExtra::LogExtra(std::initializer_list<Pair> initial,
                   ExtendType extend_type) {
  ExtendRange(initial.begin(), initial.end(), extend_type);
}

void LogExtra::Extend(std::string key, Value value, ExtendType extend_type) {
  Extend(std::move(key),
         ProtectedValue(std::move(value), extend_type == ExtendType::kFrozen));
}

void LogExtra::Extend(Pair extra, ExtendType extend_type) {
  Extend(std::move(extra.first), std::move(extra.second), extend_type);
}

void LogExtra::Extend(std::initializer_list<Pair> extra,
                      ExtendType extend_type) {
  ExtendRange(extra.begin(), extra.end(), extend_type);
}

void LogExtra::Extend(const LogExtra& extra) {
  if (extra_.empty())
    extra_ = extra.extra_;
  else
    ExtendRange(extra.extra_.begin(), extra.extra_.end());
}

void LogExtra::Extend(LogExtra&& extra) {
  if (extra_.empty()) {
    std::swap(extra_, extra.extra_);
  } else {
    for (Map::value_type& pair : extra.extra_)
      Extend(pair.first, std::move(pair.second));
    extra.extra_.clear();
  }
}

void LogExtra::SetFrozen(const std::string& key) {
  auto it = extra_.find(key);
  if (it == extra_.end())
    throw std::runtime_error("can't set frozen for non-existing key " + key);
  it->second.SetFrozen();
}

void LogExtra::Extend(std::string key, ProtectedValue protected_value,
                      ExtendType extend_type) {
  extra_[std::move(key)] =
      extend_type == ExtendType::kFrozen
          ? ProtectedValue(std::move(protected_value.GetValue()), true)
          : std::move(protected_value);
}

void LogExtra::Extend(Map::value_type extra, ExtendType extend_type) {
  Extend(std::move(extra.first), std::move(extra.second), extend_type);
}

LogExtra::ProtectedValue::ProtectedValue(Value value, bool frozen)
    : value_(std::move(value)), frozen_(frozen) {}

LogExtra::ProtectedValue& LogExtra::ProtectedValue::operator=(
    const ProtectedValue& other) {
  if (frozen_) return *this;
  value_ = other.GetValue();
  frozen_ = other.frozen_;
  return *this;
}

LogExtra::ProtectedValue& LogExtra::ProtectedValue::operator=(
    ProtectedValue&& other) {
  if (frozen_) return *this;
  value_ = std::move(other.GetValue());
  frozen_ = other.frozen_;
  return *this;
}

void LogExtra::ProtectedValue::SetFrozen() { frozen_ = true; }

}  // namespace logging
