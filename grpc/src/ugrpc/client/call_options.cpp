#include <userver/ugrpc/client/call_options.hpp>

#include <fmt/format.h>

#include <userver/utils/assert.hpp>
#include <userver/utils/text_light.hpp>

#include <userver/ugrpc/impl/to_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

namespace {

/*
 *  Custom-Metadata -> Binary-Header / ASCII-Header
 *  Binary-Header -> {Header-Name "-bin" } {binary value}
 *  ASCII-Header -> Header-Name ASCII-Value
 *  Header-Name -> 1*( %x30-39 / %x61-7A / "_" / "-" / ".") ; 0-9 a-z _ - .
 *  ASCII-Value -> 1*( %x20-%x7E ) ; space and printable ASCII
 *  Custom-Metadata -> Binary-Header / ASCII-Header
 */

bool IsValidMetadataHeaderNameSymbol(char ch) {
    return ('0' <= ch && ch <= '9') || ('a' <= ch && ch <= 'z') || '_' == ch || '-' == ch || '.' == ch;
}

bool IsValidMetadataHeaderName(std::string_view meta_key) {
    for (auto ch : meta_key) {
        if (!IsValidMetadataHeaderNameSymbol(ch)) {
            return false;
        }
    }
    return true;
}

bool IsValidMetadataAsciiValueSymbol(char ch) { return '\x20' <= ch && ch <= '\x7E'; }

bool IsValidMetadataAsciiValue(std::string_view meta_key) {
    for (auto ch : meta_key) {
        if (!IsValidMetadataAsciiValueSymbol(ch)) {
            return false;
        }
    }
    return true;
}

void CheckCustomMetadata(std::string_view meta_key, std::string_view meta_value) {
    UINVARIANT(IsValidMetadataHeaderName(meta_key), fmt::format("Invalid Custom Metadata Key: {}", meta_key));
    if (!utils::text::EndsWith(meta_key, "-bin")) {
        UINVARIANT(
            IsValidMetadataAsciiValue(meta_value),
            fmt::format("Invalid Custom Metadata Value, Key: {}", meta_key)
        );
    }
}

}  // namespace

void CallOptions::SetAttempts(int attempts) { attempts_ = attempts; }

int CallOptions::GetAttempts() const { return attempts_; }

void CallOptions::SetTimeout(std::chrono::milliseconds timeout) {
    UINVARIANT(std::chrono::milliseconds::zero() < timeout, "'timeout' should be greater than 0");
    timeout_ = timeout;
}

std::chrono::milliseconds CallOptions::GetTimeout() const { return timeout_; }

void CallOptions::SetDeadline(engine::Deadline deadline) { deadline_ = deadline; }

engine::Deadline CallOptions::GetDeadline() const { return deadline_; }

void CallOptions::AddMetadata(std::string_view meta_key, std::string_view meta_value) {
    CheckCustomMetadata(meta_key, meta_value);
    metadata_.emplace_back(ugrpc::impl::ToGrpcString(meta_key), ugrpc::impl::ToGrpcString(meta_value));
}

void CallOptions::SetClientContextFactory(ClientContextFactory&& client_context_factory) {
    client_context_factory_ = std::move(client_context_factory);
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
