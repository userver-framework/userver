#pragma once

#include <string>
#include <vector>

#include <iconv.h>

#include <boost/lockfree/stack.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::encoding {

class Converter {
public:
    Converter(std::string enc_from, std::string enc_to);
    ~Converter();

    Converter(const Converter&) = delete;
    Converter(Converter&& other) = delete;
    Converter& operator=(const Converter&) = delete;
    Converter& operator=(Converter&& other) = delete;

    bool Convert(const char* data, size_t size, std::vector<char>& out) const;

private:
    class Impl {
    public:
        struct IconvDeleter {
            void operator()(iconv_t iconv) const noexcept { ::iconv_close(iconv); }
        };
        using IconvHandle = std::unique_ptr<std::remove_pointer_t<iconv_t>, IconvDeleter>;

        Impl(std::string&& enc_from, std::string&& enc_to);
        ~Impl();

        IconvHandle Pop() const;
        void Push(IconvHandle&& handle) const;

    private:
        const std::string enc_from_;
        const std::string enc_to_;

        static const size_t kMaxIconvBufferCount = 8;
        using IconvStack = boost::lockfree::stack<iconv_t, boost::lockfree::capacity<kMaxIconvBufferCount>>;
        mutable IconvStack pool_;
    };

    Impl impl_;
};

}  // namespace utils::encoding

USERVER_NAMESPACE_END
