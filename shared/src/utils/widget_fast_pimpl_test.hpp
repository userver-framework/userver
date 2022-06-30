#include <userver/utest/using_namespace_userver.hpp>

/// [FastPimpl - header]
#pragma once

#include <userver/compiler/select.hpp>
#include <userver/utils/fast_pimpl.hpp>

namespace sample {

class Widget {
 public:
  Widget();

  Widget(Widget&& other) noexcept;
  Widget(const Widget& other);
  Widget& operator=(Widget&& other) noexcept;
  Widget& operator=(const Widget& other);
  ~Widget();

  int DoSomething(short param);

 private:
  struct Impl;

  static constexpr std::size_t kImplSize =
      compiler::SelectSize().For64Bit(8).For32Bit(4);
  static constexpr std::size_t kImplAlign = alignof(void*);
  utils::FastPimpl<Impl, kImplSize, kImplAlign, utils::kStrictMatch> pimpl_;
};

}  // namespace sample
/// [FastPimpl - header]
