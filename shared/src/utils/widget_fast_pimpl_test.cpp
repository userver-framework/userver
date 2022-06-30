/// [FastPimpl - source]
#include "widget_fast_pimpl_test.hpp"  // sample::Widget

#include "third_party_ugly.hpp"  // Some ugly headers for the implementation

namespace sample {

struct Widget::Impl {  // Implementation to hide
  int Do(short param) const;

  Ugly payload_;  // Something ugly from "third_party_ugly.hpp"
};

Widget::Widget() : pimpl_(Impl{/*some initializers*/}) {}

Widget::Widget(Widget&& other) noexcept = default;
Widget::Widget(const Widget& other) = default;
Widget& Widget::operator=(Widget&& other) noexcept = default;
Widget& Widget::operator=(const Widget& other) = default;
Widget::~Widget() = default;

int Widget::DoSomething(short param) { return pimpl_->Do(param); }

}  // namespace sample
/// [FastPimpl - source]

namespace sample {

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
int Widget::Impl::Do(short) const { return 42; }
}  // namespace sample
