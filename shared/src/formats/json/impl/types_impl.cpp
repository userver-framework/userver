#include <formats/json/impl/types_impl.hpp>

#include <utility>

#include <userver/utils/assert.hpp>
#include <userver/utils/not_null.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::impl {

namespace {

bool IsComplex(const Value& value) {
  if (value.IsObject()) {
    return value.MemberBegin() != value.MemberEnd();
  }
  if (value.IsArray()) {
    return value.Begin() != value.End();
  }
  return false;
}

class JsonTreeDestroyer final {
 public:
  explicit JsonTreeDestroyer(Value&& root)
      : root_(std::move(root)), terminal_(&root_) {
    UpdateTerminal();
  }

  void Destroy() && {
    if (&root_ == terminal_) {
      return;
    }

    do {
      MoveComplexChildrenToTerminal();
    } while (NextDepth());
  }

 private:
  void MoveComplexChildrenToTerminal() {
    UASSERT(IsComplex(root_));

    if (root_.IsObject()) {
      for (auto it = root_.MemberBegin() + 1; it != root_.MemberEnd(); ++it) {
        if (IsComplex(it->value)) {
          MoveValueToTerminal(std::move(it->value));
        }
      }
    } else {
      for (auto* it = root_.Begin() + 1; it != root_.End(); ++it) {
        if (IsComplex(*it)) {
          MoveValueToTerminal(std::move(*it));
        }
      }
    }
  }

  bool NextDepth() {
    UASSERT(IsComplex(root_));

    Value& next_depth =
        root_.IsObject() ? root_.MemberBegin()->value : *root_.Begin();

    const bool is_last_level = &next_depth == terminal_;

    Value field_to_destroy(std::move(next_depth));
    root_.Swap(field_to_destroy);

    return !is_last_level;
  }

  void MoveValueToTerminal(Value&& value) {
    *terminal_ = std::move(value);
    UpdateTerminal();
  }

  void UpdateTerminal() {
    while (IsComplex(*terminal_)) {
      if (terminal_->IsObject()) {
        terminal_ = terminal_->MemberBegin()->value;
      } else {
        terminal_ = *terminal_->Begin();
      }
    }
  }

  Value root_;
  utils::NotNull<Value*> terminal_;
};

// ~GenericValue destroys members with recursion, which causes
// stackoverflow error with deep objects
void DestroyMembersIteratively(Value&& native) noexcept {
  JsonTreeDestroyer(std::move(native)).Destroy();
}

}  // namespace

VersionedValuePtr::Data::~Data() {
  DestroyMembersIteratively(std::move(native));
}

}  // namespace formats::json::impl

USERVER_NAMESPACE_END
