#include <userver/storages/clickhouse/query.hpp>

#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse {

Query::Query(const char* text, std::optional<Query::Name> name) : text_{text}, name_{std::move(name)} {}

Query::Query(std::string text, std::optional<Query::Name> name) : text_{std::move(text)}, name_{std::move(name)} {}

const std::string& Query::QueryText() const& { return text_; }

const std::optional<Query::Name>& Query::QueryName() const& { return name_; }

inline std::size_t Query::CountBraces(std::string_view s) const {
    std::size_t count = 0;
    std::size_t pos = 0;
    while ((pos = s.find("{}", pos)) != std::string_view::npos) {
        ++count;
        pos += 2;
    }
    return count;
}

Query Query::WithArgs(const ParameterStore& params) const {
    auto expected = CountBraces(text_);
    auto actual = params.GetParameters().size();
    if (expected != actual) {
        throw std::runtime_error(fmt::format(
            "Parameter count mismatch: query ({}) expects {} placeholders, but got {} parameters.\n",
            text_,
            expected,
            actual
        ));
    }
    return Query{fmt::vformat(text_, params.GetParameters()), name_};
}

void Query::FillSpanTags(tracing::Span& span) const {
    if (name_.has_value()) {
        span.AddTag(tracing::kDatabaseStatementName, name_->GetUnderlying());
    }
}

}  // namespace storages::clickhouse

USERVER_NAMESPACE_END
