#pragma once

#include <userver/ugrpc/server/stream.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

template <typename CallTraits>
class Responder;

class NoStreamingAdapter {
public:
    virtual ~NoStreamingAdapter() = default;
};

template <typename CallTraits>
class ReaderAdapter : public Reader<typename CallTraits::Request> {
public:
    [[nodiscard]] bool Read(typename CallTraits::Request& request) override {
        return static_cast<Responder<CallTraits>&>(*this).DoRead(request);
    }
};

template <typename CallTraits>
class WriterAdapter : public Writer<typename CallTraits::Response> {
public:
    void Write(typename CallTraits::Response& response) override {
        static_cast<Responder<CallTraits>&>(*this).DoWrite(response);
    }

    void Write(typename CallTraits::Response&& response) override { Write(response); }
};

template <typename CallTraits>
class ReaderWriterAdapter : public ReaderWriter<typename CallTraits::Request, typename CallTraits::Response> {
public:
    [[nodiscard]] bool Read(typename CallTraits::Request& request) override {
        return static_cast<Responder<CallTraits>&>(*this).DoRead(request);
    }

    void Write(typename CallTraits::Response& response) override {
        static_cast<Responder<CallTraits>&>(*this).DoWrite(response);
    }

    void Write(typename CallTraits::Response&& response) override { Write(response); }
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
