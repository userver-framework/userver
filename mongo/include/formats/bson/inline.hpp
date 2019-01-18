#pragma once

/// @file formats/bson/inline.hpp
/// @brief Inline value builders

#include <formats/bson/bson_builder.hpp>
#include <formats/bson/document.hpp>
#include <formats/bson/types.hpp>
#include <formats/bson/value.hpp>
#include <utils/fast_pimpl.hpp>
#include <utils/string_view.hpp>

namespace formats::bson {

/// Constructs a Document from provided key-value pairs
template <typename... Args>
Document MakeDoc(Args&&...);

/// Constructs an array Value from provided element list
template <typename... Args>
Value MakeArray(Args&&...);

namespace impl {

class InlineDocBuilder {
 public:
  using Key = utils::string_view;

  Document Build();

  template <typename FieldValue, typename... Tail>
  Document Build(Key key, FieldValue&& value, Tail&&... tail) {
    builder_.Append(key, std::forward<FieldValue>(value));
    return Build(std::forward<Tail>(tail)...);
  }

 private:
  BsonBuilder builder_;
};

class InlineArrayBuilder {
 public:
  InlineArrayBuilder();
  ~InlineArrayBuilder();

  Value Build();

  template <typename Element, typename... Tail>
  Value Build(Element&& element, Tail&&... tail) {
    builder_.Append(GetKey(), std::forward<Element>(element));
    return Build(std::forward<Tail>(tail)...);
  }

 private:
  utils::string_view GetKey();

  class Helper;
  static constexpr size_t kSize = 20;
  static constexpr size_t kAlignment = 4;
  utils::FastPimpl<Helper, kSize, kAlignment, true> helper_;
  BsonBuilder builder_;
};

}  // namespace impl

/// @cond
template <typename... Args>
Document MakeDoc(Args&&... args) {
  return impl::InlineDocBuilder().Build(std::forward<Args>(args)...);
}

template <typename... Args>
Value MakeArray(Args&&... args) {
  return impl::InlineArrayBuilder().Build(std::forward<Args>(args)...);
}
/// @endcond

}  // namespace formats::bson
