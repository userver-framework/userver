/** @file curl-ev/initialization.hpp
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.

        Helper to automatically initialize and cleanup libcurl resources
*/

#pragma once

#include <memory>

#include "config.hpp"

namespace curl {
class initialization {
 public:
  using ptr = std::shared_ptr<initialization>;
  static ptr ensure_initialization();
  ~initialization();

 protected:
  initialization();
};
}  // namespace curl
