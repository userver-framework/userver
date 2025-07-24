#pragma once

#include <utility>
#include <vector>

#include <userver/storages/sqlite/impl/extractor_base.hpp>
#include <userver/storages/sqlite/impl/io/result_binder.hpp>
#include <userver/storages/sqlite/row_types.hpp>
#include <userver/storages/sqlite/sqlite_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

template <typename T, typename ExtractionTag>
class TypedExtractor final : public impl::ExtractorBase {
public:
    explicit TypedExtractor(impl::ResultWrapper& result_wrapper) : binder_{result_wrapper} {}

    ~TypedExtractor() final = default;

    void BindNextRow() final {
        auto& bind_value = data_.emplace_back();
        binder_.BindTo(bind_value, ExtractionTag{});
    }

    std::vector<T> ExtractData() { return std::move(data_); }

private:
    io::ResultBinder binder_;
    std::vector<T> data_;  // TODO: abstract container
};

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
