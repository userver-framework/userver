#pragma once
#include <string>
#include <userver/logging/log_extra.hpp>
#include <userver/tracing/span.hpp>
/// @file userver/tracing/scope_tag.hpp
/// @brief @copybrief tracing::ScopeTag

USERVER_NAMESPACE_BEGIN

namespace tracing {

namespace impl {
struct ScopeTagsImpl {
  ScopeTagsImpl(Span& parent, std::string key, logging::LogExtra::Value value);
  ~ScopeTagsImpl();
  bool IsRoot() const noexcept;
  void AddTag();
  void AddTagFrozen();
  static std::optional<logging::LogExtra::ProtectedValue> GetPreviousValue(
      const Span& parent, std::string_view key);

  const std::string key_;
  Span& parent_;
  const logging::LogExtra::Value my_value_;
  const std::optional<logging::LogExtra::ProtectedValue> previous_value_;
};
}  // namespace impl

class ScopeTag {
 public:
  explicit ScopeTag(std::string key, logging::LogExtra::Value value);

  explicit ScopeTag(Span& parent, std::string key,
                    logging::LogExtra::Value value);

  ~ScopeTag() = default;

  ScopeTag(const ScopeTag& other) = delete;
  ScopeTag& operator=(const ScopeTag& other) = delete;

  ScopeTag(ScopeTag&& other) = delete;
  ScopeTag& operator=(ScopeTag&& other) = delete;

 private:
  impl::ScopeTagsImpl impl_;
};

class FrozenScopeTag {
 public:
  FrozenScopeTag(std::string key, logging::LogExtra::Value value);

  explicit FrozenScopeTag(Span& parent, std::string key,
                          logging::LogExtra::Value value);

  ~FrozenScopeTag() = default;

  FrozenScopeTag(const FrozenScopeTag& other) = delete;
  FrozenScopeTag& operator=(const FrozenScopeTag& other) = delete;

  FrozenScopeTag(FrozenScopeTag&& other) = delete;
  FrozenScopeTag& operator=(FrozenScopeTag&& other) = delete;

 private:
  impl::ScopeTagsImpl impl_;
};
}  // namespace tracing

USERVER_NAMESPACE_END