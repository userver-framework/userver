#pragma once

/// @file userver/tracing/scope_tag.hpp
/// @brief @copybrief tracing::TagScope

#include <string>

#include <userver/logging/log_extra.hpp>
#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

/// @brief RAII object that calls Span::AddTag/Span::AddTagFrozen function
/// in constructor and reverts these actions in destructor.
///
/// ## Example usage:
/// @snippet tracing/tag_scope_test.cpp  TagScope - sample
class TagScope {
 public:
  explicit TagScope(std::string key, logging::LogExtra::Value value,
                    logging::LogExtra::ExtendType extend_type =
                        logging::LogExtra::ExtendType::kNormal);

  explicit TagScope(Span& parent, std::string key,
                    logging::LogExtra::Value value,
                    logging::LogExtra::ExtendType extend_type =
                        logging::LogExtra::ExtendType::kNormal);

  explicit TagScope(logging::LogExtra&& extra);

  explicit TagScope(Span& parent, logging::LogExtra&& extra);

  ~TagScope();

  TagScope(const TagScope& other) = delete;
  TagScope& operator=(const TagScope& other) = delete;

  TagScope(TagScope&& other) = delete;
  TagScope& operator=(TagScope&& other) = delete;

 private:
  void AddTag(std::string&& key, logging::LogExtra::ProtectedValue&& value);

  static constexpr std::size_t kNewKeysVectorSize = 8;

  Span& parent_;
  std::size_t new_tags_begin_index_;
  std::size_t new_tags_end_index_;
  std::vector<std::pair<std::size_t, logging::LogExtra::ProtectedValue>>
      previous_values_;
};

}  // namespace tracing

USERVER_NAMESPACE_END
