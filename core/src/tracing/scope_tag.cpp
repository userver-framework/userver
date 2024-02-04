#include <boost/container/small_vector.hpp>
#include <tracing/span_impl.hpp>
#include <userver/tracing/scope_tag.hpp>
#include <userver/utils/assert.hpp>
USERVER_NAMESPACE_BEGIN

std::optional<logging::LogExtra::ProtectedValue>
tracing::impl::ScopeTagsImpl::GetPreviousValue(const Span& parent,
                                               std::string_view key) {
  auto* node = parent.pimpl_->log_extra_inheritable_.Find(key);
  std::optional ret =
      (node == nullptr) ? std::nullopt : std::make_optional(node->second);
  return ret;
}
tracing::impl::ScopeTagsImpl::ScopeTagsImpl(Span& parent, std::string key,
                                            logging::LogExtra::Value value)
    : key_(std::move(key)),
      parent_(parent),
      my_value_(value),
      previous_value_(GetPreviousValue(parent_, key_)) {}
tracing::impl::ScopeTagsImpl::~ScopeTagsImpl() {
  auto* my_node = parent_.pimpl_->log_extra_inheritable_.Find(key_);

  if (my_node == nullptr || my_node->second.GetValue() != my_value_) {
    // value was overwritten by span::AddTag(key, new_value), or another
    // (Frozen)ScopeTag, or initially we were trying to overwrite frozen value.
    // It is logical to not overwrite new_value here
    return;
  }
  if (IsRoot()) {
    const std::ptrdiff_t idx =
        my_node - parent_.pimpl_->log_extra_inheritable_.extra_->data();
    parent_.pimpl_->log_extra_inheritable_.Erase(idx);
  } else {
    my_node->second.AssignIgnoringFrozenness(*previous_value_);
  }
}
bool tracing::impl::ScopeTagsImpl::IsRoot() const noexcept {
  return !previous_value_.has_value();
}
void tracing::impl::ScopeTagsImpl::AddTag() { parent_.AddTag(key_, my_value_); }
void tracing::impl::ScopeTagsImpl::AddTagFrozen() {
  parent_.AddTagFrozen(key_, my_value_);
}

tracing::ScopeTag::ScopeTag(std::string key, logging::LogExtra::Value value)
    : impl_(Span::CurrentSpan(), std::move(key), std::move(value)) {
  impl_.AddTag();
}

tracing::ScopeTag::ScopeTag(Span& parent, std::string key,
                            logging::LogExtra::Value value)
    : impl_(parent, std::move(key), std::move(value)) {
  impl_.AddTag();
}

tracing::FrozenScopeTag::FrozenScopeTag(std::string key,
                                        logging::LogExtra::Value value)
    : impl_(Span::CurrentSpan(), std::move(key), std::move(value)) {
  impl_.AddTagFrozen();
}
tracing::FrozenScopeTag::FrozenScopeTag(tracing::Span& parent, std::string key,
                                        logging::LogExtra::Value value)
    : impl_(parent, std::move(key), std::move(value))

{
  impl_.AddTagFrozen();
}
USERVER_NAMESPACE_END