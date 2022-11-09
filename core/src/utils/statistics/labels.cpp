#include <userver/utils/statistics/labels.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

LabelView::LabelView(const Label& label) noexcept
    : name_(label.Name()), value_(label.Value()) {}

LabelsSpan::LabelsSpan(const LabelView* begin, const LabelView* end) noexcept
    : begin_(begin), end_(end) {
  UASSERT(begin <= end);
  UASSERT(begin != nullptr || end == nullptr);
  UASSERT(end != nullptr || begin == nullptr);
}

bool operator<(const LabelView& x, const LabelView& y) noexcept {
  return x.Name() < y.Name() || (x.Name() == y.Name() && x.Value() < y.Value());
}

bool operator==(const LabelView& x, const LabelView& y) noexcept {
  return x.Name() == y.Name() && x.Value() == y.Value();
}

Label::Label(LabelView view) : name_{view.Name()}, value_{view.Value()} {}

bool operator<(const Label& x, const Label& y) noexcept {
  return x.Name() < y.Name() || (x.Name() == y.Name() && x.Value() < y.Value());
}

bool operator==(const Label& x, const Label& y) noexcept {
  return x.Name() == y.Name() && x.Value() == y.Value();
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
