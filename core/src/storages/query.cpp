#include <userver/storages/query.hpp>

#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages {

std::optional<Query::NameView> Query::GetOptionalNameView() const noexcept {
    return dynamic_.name_ ? std::optional<Query::NameView>{dynamic_.name_->GetUnderlying()} : std::nullopt;
}

utils::zstring_view Query::GetStatementView() const noexcept { return utils::zstring_view{dynamic_.statement_}; }

}  // namespace storages

USERVER_NAMESPACE_END
