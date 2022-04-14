#pragma once

#include <boost/intrusive/link_mode.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

using IntrusiveLinkMode = boost::intrusive::link_mode<
#ifdef NDEBUG
    boost::intrusive::normal_link
#else
    boost::intrusive::safe_link
#endif
    >;

}  // namespace utils::impl

USERVER_NAMESPACE_END
