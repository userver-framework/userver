#include <userver/engine/io/buffered.hpp>

#include <boost/algorithm/string/trim.hpp>

#include <engine/io/impl/buffer.hpp>
#include <userver/engine/io/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::io {

TerminatorNotFoundException::TerminatorNotFoundException()
    : IoException("EOF encountered before terminator could be found") {}

BufferedReader::BufferedReader(ReadableBasePtr source)
    : source_(std::move(source)) {}

BufferedReader::BufferedReader(ReadableBasePtr source, size_t buffer_size)
    : source_(std::move(source)), buffer_(buffer_size) {}

BufferedReader::~BufferedReader() = default;

BufferedReader::BufferedReader(BufferedReader&&) noexcept = default;
BufferedReader& BufferedReader::operator=(BufferedReader&&) noexcept = default;

bool BufferedReader::IsValid() const { return source_->IsValid(); }

std::string BufferedReader::ReadSome(size_t max_bytes, Deadline deadline) {
  if (!buffer_->AvailableReadBytes()) {
    buffer_->Reserve(1);
    FillBuffer(deadline);
  }
  std::string result(buffer_->ReadPtr(),
                     std::min(max_bytes, buffer_->AvailableReadBytes()));
  buffer_->ReportRead(result.size());
  return result;
}

std::string BufferedReader::ReadAll(size_t num_bytes, Deadline deadline) {
  if (buffer_->AvailableReadBytes() < num_bytes) {
    buffer_->Reserve(num_bytes - buffer_->AvailableReadBytes());
    while (buffer_->AvailableReadBytes() < num_bytes) {
      if (!FillBuffer(deadline)) break;
    }
  }
  std::string result(buffer_->ReadPtr(),
                     std::min(num_bytes, buffer_->AvailableReadBytes()));
  buffer_->ReportRead(result.size());
  return result;
}

std::string BufferedReader::ReadLine(Deadline deadline) {
  static const auto kPred = [](int c) {
    return c == '\n' || c == '\r' || c == EOF;
  };
  while (kPred(Peek(deadline))) Getc(deadline);
  auto result = ReadUntil(kPred, deadline);
  boost::algorithm::trim_right_if(result, kPred);
  return result;
}

std::string BufferedReader::ReadUntil(char terminator, Deadline deadline) {
  return ReadUntil([terminator](int c) { return c == terminator; }, deadline);
}

std::string BufferedReader::ReadUntil(const std::function<bool(int)>& pred,
                                      Deadline deadline) {
  bool found = false;
  size_t search_pos = 0;
  while (!found) {
    const auto* read_ptr = buffer_->ReadPtr();
    for (; !found && search_pos < buffer_->AvailableReadBytes(); ++search_pos) {
      found = pred(read_ptr[search_pos]);
    }

    if (!found) {
      buffer_->Reserve(1);
      if (!FillBuffer(deadline)) break;
    }
  }
  if (!found && !pred(EOF)) {
    throw TerminatorNotFoundException();
  }
  // pos is either incremented after found was set or equals available size
  std::string result(buffer_->ReadPtr(), search_pos);
  buffer_->ReportRead(result.size());
  return result;
}

int BufferedReader::Getc(Deadline deadline) {
  int result = Peek(deadline);
  if (result != EOF) buffer_->ReportRead(1);
  return result;
}

int BufferedReader::Peek(Deadline deadline) {
  if (!buffer_->AvailableReadBytes()) {
    buffer_->Reserve(1);
    FillBuffer(deadline);
  }
  return buffer_->AvailableReadBytes() ? *buffer_->ReadPtr() : EOF;
}

void BufferedReader::Discard(size_t num_bytes, Deadline deadline) {
  size_t discarded_bytes = std::min(num_bytes, buffer_->AvailableReadBytes());
  buffer_->ReportRead(discarded_bytes);

  try {
    while (discarded_bytes < num_bytes) {
      buffer_->Reserve(1);
      auto read_bytes = FillBuffer(deadline);
      if (!read_bytes) break;

      auto bytes_to_skip = std::min(read_bytes, num_bytes - discarded_bytes);
      buffer_->ReportRead(bytes_to_skip);
      discarded_bytes += bytes_to_skip;
    }
  } catch (const IoTimeout&) {
    throw IoTimeout(discarded_bytes);
  }
}

size_t BufferedReader::FillBuffer(Deadline deadline) {
  try {
    auto read_bytes = source_->ReadSome(
        buffer_->WritePtr(), buffer_->AvailableWriteBytes(), deadline);
    buffer_->ReportWritten(read_bytes);
    return read_bytes;
  } catch (const IoTimeout& ex) {
    buffer_->ReportWritten(ex.BytesTransferred());
    throw IoTimeout(buffer_->AvailableReadBytes());
  }
}

}  // namespace engine::io

USERVER_NAMESPACE_END
