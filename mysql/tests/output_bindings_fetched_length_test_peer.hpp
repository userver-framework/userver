#pragma once

#include <storages/mysql/impl/bindings/output_bindings.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

struct OutputBindingsFetchedLengthTestPeer final {
    static void SetColumnMaxFetchedByteLength(
        storages::mysql::impl::bindings::OutputBindings& binds,
        std::size_t pos,
        std::size_t max_byte_length
    ) {
        binds.intermediate_buffers_[pos].max_fetched_byte_length = max_byte_length;
    }

    static std::size_t IntermediateStringSize(storages::mysql::impl::bindings::OutputBindings& binds, std::size_t pos) {
        return binds.intermediate_buffers_[pos].string.size();
    }
};

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
