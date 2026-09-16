#pragma once

#include <cstdio>
#include <mutex>
#include <string_view>

#include <logging/impl/base_sink.hpp>
#include <userver/fs/blocking/c_file.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

class BufferedFileSink : public BaseSink {
public:
    explicit BufferedFileSink(const std::string& filename);
    ~BufferedFileSink() override;

    void Write(std::span<const struct iovec> logs) final;

    void Reopen(ReopenMode mode) override;

protected:
    explicit BufferedFileSink(fs::blocking::CFile&& file);

    fs::blocking::CFile& GetFile();

private:
    const std::string filename_;
    fs::blocking::CFile file_;
};

class BufferedUnownedFileSink final : public BufferedFileSink {
public:
    explicit BufferedUnownedFileSink(std::FILE* c_file);
    ~BufferedUnownedFileSink() override;
    void Reopen(ReopenMode) override;
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
