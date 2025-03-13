#pragma once

#include <utility>
#include <vector>

#include <userver/storages/sqlite/impl/result_wrapper.hpp>
#include <userver/storages/sqlite/row_types.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

class ExtractorBase {
 public:
  virtual ~ExtractorBase() = default;

  virtual void BindNextRow() = 0;
};

template <typename T, typename ExtractionTag>
class TypedExtractor final : public impl::ExtractorBase {
 public:
  TypedExtractor(impl::ResultWrapper& result_wrapper)
      : result_wrapper_(result_wrapper) {}

  ~TypedExtractor() final = default;

  void BindNextRow() final { data_.push_back(std::forward<T>(ExtractRow())); }

  std::vector<T> ExtractData() { return std::move(data_); }

 private:
  std::vector<T> data_;
  impl::ResultWrapper& result_wrapper_;

  T ExtractRow() {
    if constexpr (std::is_same_v<ExtractionTag, FieldTag>) {
      return result_wrapper_.FetchNext<T>(kFieldTag);
    } else {
      return result_wrapper_.FetchNext<T>(kRowTag);
    }
  }
};

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
