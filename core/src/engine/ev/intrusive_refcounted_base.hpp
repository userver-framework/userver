#pragma once

#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

class IntrusiveRefcountedBase
    : public boost::intrusive_ref_counter<IntrusiveRefcountedBase> {
 public:
  virtual ~IntrusiveRefcountedBase();
};
using OnRefcountedPayload = void(IntrusiveRefcountedBase&);

template <class Target>
Target PolymorphicDowncast(IntrusiveRefcountedBase& x) noexcept {
  UASSERT(dynamic_cast<std::remove_reference_t<Target>*>(&x) == &x);
  return static_cast<Target>(x);
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
