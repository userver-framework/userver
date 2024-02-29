#include <userver/tracing/tag_scope.hpp>

#include <boost/container/small_vector.hpp>

#include <tracing/span_impl.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

void TagScope::AddTag(std::string&& key,
                      logging::LogExtra::ProtectedValue&& value) {
  auto* it = parent_.pimpl_->log_extra_inheritable_.Find(key);
  if (it) {
    if (!it->second.IsFrozen()) {
      std::size_t tag_index =
          it - parent_.pimpl_->log_extra_inheritable_.extra_->data();
      previous_values_.emplace_back(std::move(tag_index),
                                    std::move(it->second));

      it->second = std::move(value);
    }
  } else {
    ++new_tags_end_index_;

    parent_.pimpl_->log_extra_inheritable_.extra_->emplace_back(
        std::move(key), std::move(value));
  }
}

TagScope::TagScope(std::string key, logging::LogExtra::Value value,
                   logging::LogExtra::ExtendType extend_type)
    : tracing::TagScope(Span::CurrentSpan(), std::move(key), std::move(value),
                        extend_type) {}

TagScope::TagScope(Span& parent, std::string key,
                   logging::LogExtra::Value value,
                   logging::LogExtra::ExtendType extend_type)
    : parent_(parent),
      new_tags_begin_index_(
          parent_.pimpl_->log_extra_inheritable_.extra_->size()),
      new_tags_end_index_(new_tags_begin_index_) {
  AddTag(std::move(key),
         logging::LogExtra::ProtectedValue(
             std::move(value),
             extend_type == logging::LogExtra::ExtendType::kFrozen));
}

TagScope::TagScope(logging::LogExtra&& extra)
    : tracing::TagScope(Span::CurrentSpan(), std::move(extra)) {}

TagScope::TagScope(Span& parent, logging::LogExtra&& extra)
    : parent_(parent),
      new_tags_begin_index_(
          parent_.pimpl_->log_extra_inheritable_.extra_->size()),
      new_tags_end_index_(new_tags_begin_index_) {
  for (auto& [key, value] : *extra.extra_) {
    AddTag(std::move(key), std::move(value));
  }
}

TagScope::~TagScope() {
  auto new_tags_begin = parent_.pimpl_->log_extra_inheritable_.extra_->begin() +
                        new_tags_begin_index_;
  parent_.pimpl_->log_extra_inheritable_.extra_->erase(
      new_tags_begin,
      new_tags_begin + (new_tags_end_index_ - new_tags_begin_index_));

  for (auto& [tag_index, previous_value] : previous_values_) {
    auto it =
        parent_.pimpl_->log_extra_inheritable_.extra_->begin() + tag_index;
    it->second.AssignIgnoringFrozenness(std::move(previous_value));
  }
}

}  // namespace tracing

USERVER_NAMESPACE_END
