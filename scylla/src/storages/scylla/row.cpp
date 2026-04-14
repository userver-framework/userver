#include <userver/storages/scylla/row.hpp>

#include <string>

#include <userver/storages/scylla/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla {

bool Row::Has(std::string_view name) const noexcept {
    return Find(name) != nullptr;
}

const Value* Row::Find(std::string_view name) const noexcept {
    for (const auto& cell : cells_) {
        if (cell.first == name) return &cell.second;
    }
    return nullptr;
}

const Value& Row::At(std::string_view name) const {
    const auto* v = Find(name);
    if (v == nullptr) {
        throw QueryException(std::string{"Row::At: column '"} + std::string{name} + "' not found");
    }
    return *v;
}

bool Row::IsNull(std::string_view name) const noexcept {
    const auto* v = Find(name);
    return v == nullptr || v->IsNull();
}

}  // namespace storages::scylla

USERVER_NAMESPACE_END
