#pragma once

#include <optional>

#include <boost/container/small_vector.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::parse {

template <typename ValueFrom, typename ValueToBuilder>
class ConversionStack final {
 public:
  explicit ConversionStack(const ValueFrom& value) : stack_() {
    stack_.emplace_back(value);
  }

  template <typename T>
  void CastCurrentPrimitive() {
    stack_.back().to.emplace(stack_.back().from.template As<T>());
    stack_.back().is_parsed = true;
  }

  template <typename T>
  void SetCurrent(const T& v) {
    stack_.back().to.emplace(v);
    stack_.back().is_parsed = true;
  }

  const ValueFrom& GetNextFrom() {
    if (stack_.front().is_parsed) {
      return stack_.front().from;
    } else if (!stack_.back().is_parsed) {
      return stack_.back().from;
    } else {
      return stack_[stack_.size() - 2].from;
    }
  }

  std::optional<ValueToBuilder> GetValueIfParsed() {
    if (stack_.front().is_parsed) {
      return std::move(stack_.front().to);
    }
    return std::nullopt;
  }

  void ParseNext() {
    if (!stack_.back().is_parsed) {
      if (!stack_.back().to) {
        if (stack_.back().from.IsObject()) {
          stack_.back().to.emplace(common::Type::kObject);
        } else if (stack_.back().from.IsArray()) {
          stack_.back().to.emplace(common::Type::kArray);
        } else {
          UINVARIANT(false, "Type mismatch");
        }
      }
      if (stack_.back().current_parsing_elem) {
        ++*stack_.back().current_parsing_elem;
      } else {
        stack_.back().current_parsing_elem = stack_.back().from.begin();
      }
      if (stack_.back().current_parsing_elem.value() ==
          stack_.back().from.end()) {
        stack_.back().is_parsed = true;
        return;
      }
      auto new_from = *stack_.back().current_parsing_elem.value();
      stack_.emplace_back(std::move(new_from));
      return;
    }
    UASSERT(stack_.size() > 1);
    auto& current_docs = stack_[stack_.size() - 2];
    if (current_docs.from.IsObject()) {
      current_docs.to.value()[current_docs.current_parsing_elem->GetName()] =
          std::move(stack_.back().to.value());
    } else if (current_docs.from.IsArray()) {
      current_docs.to->PushBack(std::move(stack_.back().to.value()));
    } else {
      UASSERT_MSG(false, "Type mismatch");
    }
    stack_.pop_back();
  }

 private:
  struct StackFrame final {
    explicit StackFrame(ValueFrom&& from) : from(std::move(from)) {}
    explicit StackFrame(const ValueFrom& from) : from(from) {}

    const ValueFrom from;
    std::optional<ValueToBuilder> to{};
    std::optional<typename ValueFrom::const_iterator> current_parsing_elem{};
    bool is_parsed{false};
  };

  boost::container::small_vector<StackFrame, 10> stack_;
};

}  // namespace formats::parse

USERVER_NAMESPACE_END
