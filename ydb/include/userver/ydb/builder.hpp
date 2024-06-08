#pragma once

#include <ydb-cpp-sdk/client/params/params.h>

#include <string>
#include <type_traits>
#include <utility>

#include <userver/ydb/impl/cast.hpp>
#include <userver/ydb/io/traits.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

class TableClient;
class Transaction;

class PreparedArgsBuilder final {
 public:
  PreparedArgsBuilder(PreparedArgsBuilder&&) noexcept = default;
  PreparedArgsBuilder& operator=(PreparedArgsBuilder&&) = delete;

  /// Supported types and required includes are documented in:
  /// <userver/ydb/io/supported_types.hpp>
  template <typename T>
  void Add(const std::string& name, T&& value);

  /// @cond
  // For internal use only.
  explicit PreparedArgsBuilder(NYdb::TParamsBuilder&& builder)
      : builder_(std::move(builder)) {}

  // For internal use only.
  template <typename... NamesValues>
  void AddParams(NamesValues&&... names_values);
  /// @endcond

 private:
  friend class Transaction;
  friend class TableClient;
  struct PreparedArgsWithKey;

  NYdb::TParams Build() && { return std::move(builder_).Build(); }

  PreparedArgsWithKey operator<<(const std::string& key);

  NYdb::TParamsBuilder builder_;
};

template <typename T>
void PreparedArgsBuilder::Add(const std::string& name, T&& value) {
  auto& param_builder = builder_.AddParam(impl::ToString(name));
  Write(param_builder, std::forward<T>(value));
  param_builder.Build();
}

template <typename... NamesValues>
void PreparedArgsBuilder::AddParams(NamesValues&&... names_values) {
  [[maybe_unused]] decltype(auto) result =
      (*this << ... << std::forward<NamesValues>(names_values));
  static_assert(std::is_same_v<decltype(result), PreparedArgsBuilder&>);
}

struct PreparedArgsBuilder::PreparedArgsWithKey final {
  PreparedArgsBuilder& builder;
  const std::string& key;

  template <typename T>
  PreparedArgsBuilder& operator<<(T&& value) const {
    builder.Add(key, std::forward<T>(value));
    return builder;
  }
};

inline auto PreparedArgsBuilder::operator<<(const std::string& key)
    -> PreparedArgsWithKey {
  return PreparedArgsWithKey{*this, key};
}

}  // namespace ydb

USERVER_NAMESPACE_END
