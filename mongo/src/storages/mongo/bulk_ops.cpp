#include <userver/storages/mongo/bulk_ops.hpp>

#include <storages/mongo/bulk_ops_impl.hpp>
#include <storages/mongo/operations_common.hpp>

#include <userver/utils/string_literal.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::bulk_ops {

namespace {

void AppendArrayFilters(formats::bson::impl::BsonBuilder& builder, const options::ArrayFilters& filters) {
    static constexpr utils::StringLiteral kOptionName = "arrayFilters";
    builder.Append(kOptionName, filters.Value());
}

}  // namespace

InsertOne::InsertOne(formats::bson::Document document) : impl_(std::move(document)) {}

InsertOne::~InsertOne() = default;

InsertOne::InsertOne(const InsertOne&) = default;
InsertOne::InsertOne(InsertOne&&) noexcept = default;
InsertOne& InsertOne::operator=(const InsertOne&) = default;
InsertOne& InsertOne::operator=(InsertOne&&) noexcept = default;

ReplaceOne::ReplaceOne(formats::bson::Document selector, formats::bson::Document replacement)
    : impl_(std::move(selector), std::move(replacement)) {}

ReplaceOne::~ReplaceOne() = default;

ReplaceOne::ReplaceOne(const ReplaceOne&) = default;
ReplaceOne::ReplaceOne(ReplaceOne&&) noexcept = default;
ReplaceOne& ReplaceOne::operator=(const ReplaceOne&) = default;
ReplaceOne& ReplaceOne::operator=(ReplaceOne&&) noexcept = default;

void ReplaceOne::SetOption(options::Upsert) { impl::AppendUpsert(impl::EnsureBuilder(impl_->options)); }

Update::Update(Mode mode, formats::bson::Document selector, formats::bson::Document update)
    : impl_(mode, std::move(selector), std::move(update)) {}

Update::~Update() = default;

Update::Update(const Update&) = default;
Update::Update(Update&&) noexcept = default;
Update& Update::operator=(const Update&) = default;
Update& Update::operator=(Update&&) noexcept = default;

void Update::SetOption(options::Upsert) { impl::AppendUpsert(impl::EnsureBuilder(impl_->options)); }

void Update::SetOption(const options::ArrayFilters& filters) {
    AppendArrayFilters(impl::EnsureBuilder(impl_->options), filters);
}

void Update::SetOption(const options::Hint& hint) { impl::AppendHint(impl::EnsureBuilder(impl_->options), hint); }

Delete::Delete(Mode mode, formats::bson::Document selector) : impl_(mode, std::move(selector)) {}

Delete::~Delete() = default;

Delete::Delete(const Delete&) = default;
Delete::Delete(Delete&&) noexcept = default;
Delete& Delete::operator=(const Delete&) = default;
Delete& Delete::operator=(Delete&&) noexcept = default;

void Delete::SetOption(const options::Hint& hint) { impl::AppendHint(impl::EnsureBuilder(impl_->options), hint); }

}  // namespace storages::mongo::bulk_ops

USERVER_NAMESPACE_END
