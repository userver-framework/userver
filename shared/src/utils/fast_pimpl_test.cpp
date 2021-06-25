/// [FastPimpl - usage]
#include "widget_fast_pimpl_test.hpp"  // sample::Widget

#include <gtest/gtest.h>

TEST(FastPimpl, SampleWidget) {
  sample::Widget widget;
  auto widget_copy = widget;
  EXPECT_EQ(widget_copy.DoSomething(2), 42);
}
/// [FastPimpl - usage]
