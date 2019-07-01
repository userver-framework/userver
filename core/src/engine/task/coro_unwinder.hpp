#pragma once

namespace engine {
namespace impl {

// we don't use native boost.coroutine stack unwinding mechanisms for cancel
// as they don't allow us to yield from unwind
// e.g. to serialize nested tasks destruction
//
// XXX: Standard library has catch-all in some places, namely streams.
// With libstdc++ we can use magic __forced_unwind exception class
// but libc++ has nothing of the like.
// It's possile that we will have to use low-level C++ ABI for forced unwinds
// here, but for now this'll suffice.
class CoroUnwinder final {};

}  // namespace impl
}  // namespace engine
