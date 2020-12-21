#pragma once

#include <fs/blocking/read.hpp>
#include <fs/blocking/write.hpp>
#include <utest/utest.hpp>

#include <cache/dump/operations.hpp>

namespace cache::dump {

namespace impl {

// See implementation in prepare_temp_file.cpp
std::string PrepareTempFile();

}  // namespace impl

/// Converts to binary using `Write(Writer&, const T&)`
template <typename T>
std::string ToBinary(const T& value) {
  const auto path = impl::PrepareTempFile();

  Writer writer(path);
  writer.Write(value);
  writer.Finish();

  return fs::blocking::ReadFileContents(path);
}

/// Converts from binary using `Read(Reader&, To<T>)`
template <typename T>
T FromBinary(std::string_view data) {
  const auto path = impl::PrepareTempFile();
  fs::blocking::RewriteFileContents(path, data);

  Reader reader(path);
  T value = reader.Read<T>();
  reader.Finish();

  return value;
}

/// Write a value to a cache dump and immediately read it back. If `Write` and
/// `Read` are implemented correctly, the result should be equal to the original
/// value.
template <typename T>
T WriteRead(const T& value) {
  const auto path = impl::PrepareTempFile();

  Writer writer(path);
  writer.Write(value);
  writer.Finish();

  Reader reader(path);
  T result = reader.Read<T>();
  reader.Finish();

  return result;
}

}  // namespace cache::dump
