#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <userver/storages/scylla/exception.hpp>
#include <userver/storages/scylla/value.hpp>
#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla {

class Row {
public:
    using Cell = std::pair<std::string, Value>;
    using Cells = std::vector<Cell>;
    using const_iterator = Cells::const_iterator;

    Row() = default;
    explicit Row(Cells cells)
        : cells_(std::move(cells))
    {}

    bool Empty() const noexcept { return cells_.empty(); }
    std::size_t Size() const noexcept { return cells_.size(); }

    const_iterator begin() const noexcept { return cells_.begin(); }
    const_iterator end() const noexcept { return cells_.end(); }

    const Cells& Cells_() const noexcept { return cells_; }

    bool Has(std::string_view name) const noexcept;
    const Value* Find(std::string_view name) const noexcept;
    const Value& At(std::string_view name) const;

    bool IsNull(std::string_view name) const noexcept;

    template <typename T>
    T Get(std::string_view name) const;

    template <typename T>
    std::optional<T> TryGet(std::string_view name) const;

    template <typename T>
    T As() const;

private:
    Cells cells_;
};

using Rows = std::vector<Row>;

template <typename T>
T Row::Get(std::string_view name) const {
    const auto& v = At(name);
    if (v.IsNull()) {
        throw QueryException(utils::StrCat("Row::Get<", name, ">: column is NULL"));
    }
    if (!v.Is<T>()) {
        throw QueryException(utils::StrCat("Row::Get<", name, ">: type mismatch"));
    }
    return v.Get<T>();
}

template <typename T>
std::optional<T> Row::TryGet(std::string_view name) const {
    const auto* v = Find(name);
    if (v == nullptr || v->IsNull() || !v->Is<T>()) {
        return std::nullopt;
    }
    return v->Get<T>();
}

template <typename T>
T Row::As() const {
    T out{};
    DecodeRow(*this, out);
    return out;
}

}  // namespace storages::scylla

USERVER_NAMESPACE_END
