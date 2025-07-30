#include <userver/storages/query.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages {

struct Query::NameViewVisitor {
    std::optional<Query::NameView> operator()(const DynamicStrings& x) const noexcept {
        return (x.name_ ? std::optional<Query::NameView>{x.name_->GetUnderlying()} : std::nullopt);
    }

    std::optional<Query::NameView> operator()(const StaticStrings& x) const noexcept {
        return std::optional<Query::NameView>{x.name_};
    }
};

std::optional<Query::NameView> Query::GetOptionalNameView() const noexcept {
    return std::visit(NameViewVisitor{}, data_);
}

utils::zstring_view Query::GetStatementView() const noexcept {
    return std::visit([](const auto& x) { return utils::zstring_view{x.statement_}; }, data_);
}

}  // namespace storages

USERVER_NAMESPACE_END
