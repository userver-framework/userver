#pragma once

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

namespace bindings {
class InputBindings;
class OutputBindings;
}  // namespace bindings

using InputBindingsPimpl = utils::FastPimpl<bindings::InputBindings, 1552, 8>;
using OutputBindingsPimpl = utils::FastPimpl<bindings::OutputBindings, 1944, 8>;

using InputBindingsFwd = bindings::InputBindings;
using OutputBindingsFwd = bindings::OutputBindings;

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
