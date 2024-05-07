#pragma once

#include <ydb-cpp-sdk/client/value/value.h>

#include <iterator>
#include <type_traits>
#include <vector>  // for InsertRow

#if __cpp_lib_ranges >= 201911L
#include <ranges>
#endif

#include <userver/utils/assert.hpp>
#include <userver/utils/meta.hpp>

#include <userver/ydb/io/traits.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

namespace impl {

template <typename T>
class ParseItemsIterator final {
 public:
  using difference_type = std::ptrdiff_t;
  using value_type = T;
  using reference = T;
  using iterator_category = std::input_iterator_tag;

  ParseItemsIterator() = default;

  ParseItemsIterator(NYdb::TValueParser& parser, const ParseContext& context)
      : parser_(&parser), context_(&context) {
    ++*this;
  }

  // Trivially copyable.
  ParseItemsIterator(const ParseItemsIterator&) = default;
  ParseItemsIterator& operator=(const ParseItemsIterator&) = default;

  T operator*() const {
    UASSERT(parser_ && context_);
    return ydb::Parse<T>(*parser_, *context_);
  }

  ParseItemsIterator& operator++() {
    UASSERT(parser_ && context_);
    if (!parser_->TryNextListItem()) {
      parser_ = nullptr;
    }
    return *this;
  }

  bool operator==(const ParseItemsIterator& other) const noexcept {
    UASSERT(parser_ == nullptr || other.parser_ == nullptr);
    return parser_ == other.parser_;
  }

 private:
  NYdb::TValueParser* parser_{nullptr};
  const ParseContext* context_{nullptr};
};

}  // namespace impl

/// @cond
struct InsertColumn;
using InsertRow = std::vector<InsertColumn>;
/// @endcond

template <typename T>
struct ValueTraits<T, std::enable_if_t<meta::kIsRange<T> && !meta::kIsMap<T>>> {
  using ValueType = meta::RangeValueType<T>;

  static T Parse(NYdb::TValueParser& parser, const ParseContext& context) {
    parser.OpenList();
#if __cpp_lib_ranges_to_container >= 202202L
    T result =
        std::ranges::subrange{
            impl::ParseItemsIterator<ValueType>{parser, context},
            impl::ParseItemsIterator<ValueType>{},
        } |
        std::ranges::to<T>();
#else
    T result(impl::ParseItemsIterator<ValueType>{parser, context},
             impl::ParseItemsIterator<ValueType>{});
#endif
    parser.CloseList();
    return result;
  }

  template <typename Builder, typename U = const T&>
  static void Write(NYdb::TValueBuilderBase<Builder>& builder, U&& value) {
    bool list_opened = false;
    for (auto&& item : value) {
      if (!list_opened) {
        builder.BeginList();
        list_opened = true;
      }
      builder.AddListItem();
      ydb::Write(builder, item);
    }
    if (list_opened) {
      builder.EndList();
    } else if constexpr (std::is_same_v<ValueType, InsertRow>) {
      // Legacy behavior. Sometimes YDB will throw here due to insufficient type
      // info.
      builder.EmptyList();
    } else {
      builder.EmptyList(ValueTraits<ValueType>::MakeType());
    }
  }

  static NYdb::TType MakeType() {
    NYdb::TTypeBuilder builder;
    builder.List(ValueTraits<ValueType>::MakeType());
    return builder.Build();
  }
};

}  // namespace ydb

USERVER_NAMESPACE_END
