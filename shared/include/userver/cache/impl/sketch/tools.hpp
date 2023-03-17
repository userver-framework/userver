#pragma once

#include <cstddef>

USERVER_NAMESPACE_BEGIN

namespace cache::impl::sketch {

template <typename T, typename Hash>
class Hashes {
    public:
        Hashes(const T& element, Hash hash = Hash{});
    private:
        
};

size_t NextPowerOfTwo(size_t n);
}  // namespace cache::impl::sketch

USERVER_NAMESPACE_END
