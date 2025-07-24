#pragma once

#include <boost/pfr/core.hpp>

#include <userver/storages/sqlite/exceptions.hpp>
#include <userver/storages/sqlite/impl/binder_fwd.hpp>
#include <userver/storages/sqlite/impl/io/binder_declarations.hpp>
#include <userver/storages/sqlite/row_types.hpp>
#include <userver/storages/sqlite/sqlite_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl::io {

class ResultBinder final {
public:
    explicit ResultBinder(impl::ResultWrapper& result_wrapper);
    ~ResultBinder();

    ResultBinder(const ResultBinder& other) = delete;
    ResultBinder(ResultBinder&& other) noexcept;

    int ColumnCount();

    template <typename T, typename ExtractionTag>
    void BindTo(T& row, ExtractionTag) {
        if constexpr (std::is_same_v<ExtractionTag, RowTag>) {
            if constexpr (std::is_aggregate_v<T>) {
                boost::pfr::for_each_field(row, [&binds = GetBinds()](auto& field, std::size_t i) {
                    storages::sqlite::impl::io::BindOutput(binds, i, field);
                });
            } else if constexpr (IsTuple<T>()) {
                std::apply(
                    [&binds = GetBinds(), i = 0](auto&&... field) mutable {
                        (storages::sqlite::impl::io::BindOutput(binds, i++, field), ...);
                    },
                    row
                );
            }
        } else {
            static_assert(std::is_same_v<ExtractionTag, FieldTag>);
            // TODO: exception or abort == UASSERT(ColumnCount() == 1); ?
            if (ColumnCount() != 1) {
                throw SQLiteException{"Result set must have exactly one column for AsVector(FieldTag) "};
            }
            storages::sqlite::impl::io::BindOutput(GetBinds(), 0, row);
        }
    }

    OutputBindingsFwd& GetBinds();

private:
    OutputBindingsPimpl impl_;
};

}  // namespace storages::sqlite::impl::io

USERVER_NAMESPACE_END
