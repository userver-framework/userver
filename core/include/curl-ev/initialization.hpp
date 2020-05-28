/** @file curl-ev/initialization.hpp
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.

        Helper to automatically initialize and cleanup libcurl resources
*/

#pragma once

#include <memory>

namespace curl {
class initialization final {
 public:
  using ptr = std::shared_ptr<initialization>;
  static ptr ensure_initialization();
  ~initialization();

 protected:
  initialization();
};
}  // namespace curl
