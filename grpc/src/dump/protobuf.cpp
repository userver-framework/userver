#include <userver/dump/protobuf.hpp>

#include <fmt/format.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/message_lite.h>
#include <google/protobuf/util/delimited_message_util.h>
#include <boost/container/small_vector.hpp>

#include <userver/compiler/demangle.hpp>
#include <userver/dump/operations.hpp>
#include <userver/dump/unsafe.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump::impl {

namespace {
constexpr std::size_t kChunkSize = 1024;
constexpr std::size_t kDelimitedPrefixSize = 10;
}  // namespace

void WriteProtoMessageToDump(Writer& writer, const google::protobuf::MessageLite& message) {
    const auto buffer_size = static_cast<std::size_t>(message.ByteSizeLong()) + kDelimitedPrefixSize;
    // TODO remove a copy on writes, add DumpZeroCopyOutputStream
    boost::container::small_vector<char, 1024> buffer{buffer_size, boost::container::default_init};
    google::protobuf::io::ArrayOutputStream output{buffer.data(), static_cast<int>(buffer_size)};

    if (!google::protobuf::util::SerializeDelimitedToZeroCopyStream(message, &output)) {
        throw dump::Error{fmt::format("Failed to serialize protobuf {}", message.GetTypeName())};
    }

    dump::WriteStringViewUnsafe(writer, std::string_view{buffer.data(), static_cast<std::size_t>(output.ByteCount())});
}

class DumpZeroCopyInputStream final : public google::protobuf::io::ZeroCopyInputStream {
public:
    explicit DumpZeroCopyInputStream(dump::Reader& reader) : reader_(reader) {}

    bool Next(const void** data, int* size) override {
        const auto buffer_view = dump::ReadUnsafeAtMost(reader_, kChunkSize);
        if (buffer_view.empty()) {
            return false;
        }
        *data = buffer_view.data();
        *size = buffer_view.size();
        global_position_ += buffer_view.size();
        return true;
    }

    void BackUp(int count) override {
        const auto size = static_cast<std::size_t>(count);
        dump::BackUpReadUnsafe(reader_, size);
        global_position_ -= size;
    }

    bool Skip(int count) override {
        const auto size_requested = static_cast<std::size_t>(count);
        const auto buffer_view = dump::ReadUnsafeAtMost(reader_, count);
        global_position_ += buffer_view.size();
        if (buffer_view.size() != size_requested) {
            UASSERT(buffer_view.size() < size_requested);
            return false;
        }
        return true;
    }

    int64_t ByteCount() const override { return global_position_; }

private:
    dump::Reader& reader_;
    std::size_t global_position_{0};
};

void ParseProtoMessageFromDump(Reader& reader, google::protobuf::MessageLite& message) {
    DumpZeroCopyInputStream input{reader};

    if (!google::protobuf::util::ParseDelimitedFromZeroCopyStream(&message, &input, nullptr)) {
        throw dump::Error{fmt::format("Failed to parse protobuf {}", message.GetTypeName())};
    }
}

}  // namespace dump::impl

USERVER_NAMESPACE_END
