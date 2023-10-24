#pragma once

/// @file userver/formats/common/conversion_stack.hpp
/// @brief @copybrief formats::common::ConversionStack

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include <fmt/format.h>
#include <boost/container/small_vector.hpp>

#include <userver/compiler/demangle.hpp>
#include <userver/formats/common/type.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::common {

/// @brief Used in the implementation of functions that convert between
/// different formats, e.g. formats::yaml::Value to formats::json::Value, etc.
///
/// Does not use recursion, so stack overflow will never happen with deeply
/// nested or wide objects or arrays.
///
/// @tparam ValueFrom the Value type, FROM which we convert,
/// e.g. formats::yaml::Value
/// \tparam ValueToBuilder the ValueBuilder type, TO which we convert,
/// e.g. formats::json::ValueBuilder
///
/// How to build the conversion function:
/// 1. Start by constructing the ConversionStack from the source `ValueFrom`
/// 2. While `!IsParsed`:
///    1. call `GetNextFrom`
///    2. analyse the kind of the current `ValueFrom`
///    3. call one of:
///       - CastCurrentPrimitive
///       - SetCurrent
///       - EnterItems
/// 3. Call `GetParsed`
///
/// See the implementation of PerformMinimalFormatConversion as an example.
template <typename ValueFrom, typename ValueToBuilder>
class ConversionStack final {
 public:
  /// Start the conversion from `value`.
  explicit ConversionStack(ValueFrom value) : stack_() {
    stack_.emplace_back(std::move(value));
  }

  ConversionStack(ConversionStack&&) = delete;
  ConversionStack& operator=(ConversionStack&&) = delete;

  /// Check whether the whole source ValueFrom has been parsed.
  bool IsParsed() const { return stack_.front().is_parsed; }

  /// Get the parsing result, precondition: `IsParsed()`.
  ValueToBuilder&& GetParsed() && {
    UASSERT(IsParsed());
    return std::move(stack_.front().to).value();
  }

  /// Get the current sub-`ValueFrom` to convert.
  const ValueFrom& GetNextFrom() const {
    if (stack_.front().is_parsed) {
      return stack_.front().from;
    } else if (!stack_.back().is_parsed) {
      return stack_.back().from;
    } else {
      return stack_[stack_.size() - 2].from;
    }
  }

  /// Use for when it's discovered that the current `ValueFrom` type is
  /// available and has the same semantics in ValueFrom and ValueToBuilder,
  /// e.g. `std::int64_t`, `std::string`, etc.
  template <typename T>
  void CastCurrentPrimitive() {
    UASSERT(!stack_.front().is_parsed && !stack_.back().is_parsed);
    SetCurrent(stack_.back().from.template As<T>());
  }

  /// Use for when it's discovered that the current `ValueFrom` type is not
  /// directly convertible to `ValueToBuilder`, but can be converted manually
  /// (this might be an inexact conversion). For example, ValueFrom could have a
  /// special representation for datetime, and we manually convert it to string
  /// using some arbitrary format.
  template <typename T>
  void SetCurrent(T&& value) {
    stack_.back().to.emplace(std::forward<T>(value));
    stack_.back().is_parsed = true;
  }

  /// Use for when an object or an array `ValueFrom` is encountered.
  void EnterItems() {
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

/// @brief Performs the conversion between different formats. Only supports
/// basic formats node types, throws on any non-standard ones.
///
/// @note This is intended as a building block for conversion functions. Prefer
/// calling `value.As<AnotherFormat>()` or `value.ConvertTo<AnotherFormat>()`.
template <typename ValueTo, typename ValueFrom>
ValueTo PerformMinimalFormatConversion(ValueFrom&& value) {
  if (value.IsMissing()) {
    throw typename std::decay_t<ValueFrom>::Exception(fmt::format(
        "Failed to convert value at '{}' from {} to {}: missing value",
        value.GetPath(), compiler::GetTypeName<std::decay_t<ValueFrom>>(),
        compiler::GetTypeName<ValueTo>()));
  }
  formats::common::ConversionStack<std::decay_t<ValueFrom>,
                                   typename ValueTo::Builder>
      conversion_stack(std::forward<ValueFrom>(value));
  while (!conversion_stack.IsParsed()) {
    const auto& from = conversion_stack.GetNextFrom();
    if (from.IsBool()) {
      conversion_stack.template CastCurrentPrimitive<bool>();
    } else if (from.IsInt()) {
      conversion_stack.template CastCurrentPrimitive<int>();
    } else if (from.IsInt64()) {
      conversion_stack.template CastCurrentPrimitive<std::int64_t>();
    } else if (from.IsUInt64()) {
      conversion_stack.template CastCurrentPrimitive<std::uint64_t>();
    } else if (from.IsDouble()) {
      conversion_stack.template CastCurrentPrimitive<double>();
    } else if (from.IsString()) {
      conversion_stack.template CastCurrentPrimitive<std::string>();
    } else if (from.IsNull()) {
      conversion_stack.SetCurrent(common::Type::kNull);
    } else if (from.IsArray() || from.IsObject()) {
      conversion_stack.EnterItems();
    } else {
      throw typename std::decay_t<ValueFrom>::Exception(fmt::format(
          "Failed to convert value at '{}' from {} to {}: unknown node type",
          from.GetPath(), compiler::GetTypeName<std::decay_t<ValueFrom>>(),
          compiler::GetTypeName<ValueTo>()));
    }
  }
  return std::move(conversion_stack).GetParsed().ExtractValue();
}

}  // namespace formats::common

USERVER_NAMESPACE_END
