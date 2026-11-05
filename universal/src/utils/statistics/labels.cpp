#include <userver/utils/statistics/labels.hpp>

#include <concepts>

#include <fmt/ranges.h>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

LabelView::LabelView(const Label& label)
    : name_(label.Name()),
      value_(label.Value())
{
    UINVARIANT(!name_.empty(), "The label name must not be empty.");
}

LabelsSpan::LabelsSpan(const LabelView* begin, const LabelView* end) noexcept : begin_(begin), end_(end) {
    UASSERT(begin <= end);
    UASSERT(begin != nullptr || end == nullptr);
    UASSERT(end != nullptr || begin == nullptr);
}

std::strong_ordering operator<=>(const LabelView& x, const LabelView& y) noexcept {
    const auto name_order = x.Name() <=> y.Name();
    if (name_order != 0) {
        return name_order;
    }
    return x.Value() <=> y.Value();
}

bool operator==(const LabelView& x, const LabelView& y) noexcept {
    return x.Name() == y.Name() && x.Value() == y.Value();
}

Label::Label(LabelView view)
    : name_{view.Name()},
      value_{view.Value()}
{}

Label::Label(std::string name, std::string value)
    : name_(std::move(name)),
      value_(std::move(value))
{
    UASSERT(!name_.empty());
}

std::strong_ordering operator<=>(const Label& x, const Label& y) noexcept {
    const auto name_order = x.Name() <=> y.Name();
    if (name_order != 0) {
        return name_order;
    }
    return x.Value() <=> y.Value();
}

bool operator==(const Label& x, const Label& y) noexcept { return x.Name() == y.Name() && x.Value() == y.Value(); }

static_assert(std::totally_ordered<LabelView>);
static_assert(std::totally_ordered<Label>);

}  // namespace utils::statistics

USERVER_NAMESPACE_END

fmt::format_context::iterator fmt::formatter<USERVER_NAMESPACE::utils::statistics::LabelsSpan>::format(
    USERVER_NAMESPACE::utils::statistics::LabelsSpan value,
    format_context& ctx
) const {
    return fmt::format_to(ctx.out(), "{}", fmt::join(value, ";"));
}
