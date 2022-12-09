#include <storages/mysql/impl/mysql_binds_storage.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

namespace {

void UpdateBindBufferLength(MYSQL_BIND& bind, enum_field_types type) {
  const auto type_size = [type]() -> std::size_t {
    switch (type) {
      case MYSQL_TYPE_TINY:
        return 1;
      case MYSQL_TYPE_SHORT:
        return 2;
      case MYSQL_TYPE_LONG:
        return 4;
      case MYSQL_TYPE_LONGLONG:
        return 8;
      case MYSQL_TYPE_FLOAT:
        return 4;
      case MYSQL_TYPE_DOUBLE:
        return 8;

      default:
        return 0;
    };
  }();

  if (type_size != 0) {
    bind.buffer_length = type_size;
  }
}

}  // namespace

BindsStorage::BindsStorage(BindsType binds_type) : binds_type_{binds_type} {}

BindsStorage::~BindsStorage() = default;

void BindsStorage::Bind(std::size_t pos, std::int8_t& val) {
  DoBind(pos, MYSQL_TYPE_TINY, &val, 0);
}

void BindsStorage::Bind(std::size_t pos, std::uint8_t& val) {
  DoBind(pos, MYSQL_TYPE_TINY, &val, 0, true);
}

void BindsStorage::Bind(std::size_t pos, std::int32_t& val) {
  DoBind(pos, MYSQL_TYPE_LONG, &val, 0);
}

void BindsStorage::Bind(std::size_t pos, std::uint32_t& val) {
  DoBind(pos, MYSQL_TYPE_LONG, &val, 0, true);
}

void BindsStorage::Bind(std::size_t pos, std::string& val) {
  if (binds_type_ == BindsType::kParameters) {
    DoBind(pos, MYSQL_TYPE_STRING, val.data(), val.length());
  } else {
    DoBindOutputString(pos, val);
  }
}

void BindsStorage::Bind(std::size_t pos, std::string_view& val) {
  UINVARIANT(binds_type_ != BindsType::kResult,
             "std::string_view is not supported in result extraction");

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  DoBind(pos, MYSQL_TYPE_STRING, const_cast<char*>(val.data()), val.length());
}

void BindsStorage::ResizeOutputString(std::size_t pos) {
  UASSERT(binds_type_ == BindsType::kResult);

  auto& output_string = output_strings_[pos];
  auto& bind = binds_[pos];

  output_string.output->resize(output_string.output_length);
  bind.buffer = output_string.output->data();
  bind.buffer_length = output_string.output->size();
}

void BindsStorage::FixupForInsert(std::size_t pos, char* buffer,
                                  std::size_t* lengths) {
  UASSERT(binds_type_ == BindsType::kParameters);

  auto& bind = binds_[pos];
  bind.buffer = buffer;
  bind.length = lengths;
}

MYSQL_BIND* BindsStorage::GetBindsArray() { return binds_.data(); }

std::size_t BindsStorage::Size() const { return binds_.size(); }

bool BindsStorage::Empty() const { return binds_.empty(); }

void BindsStorage::ResizeIfNecessary(std::size_t pos) {
  if (pos >= binds_.size()) {
    const auto new_size = pos + 1;
    binds_.resize(new_size);

    if (binds_type_ == BindsType::kResult) {
      output_strings_.resize(new_size);
    }
  }
}

void BindsStorage::DoBindOutputString(std::size_t pos, std::string& val) {
  ResizeIfNecessary(pos);

  auto& output_string = output_strings_[pos];
  output_string.output = &val;

  auto& bind = binds_[pos];
  bind.buffer_type = MYSQL_TYPE_STRING;
  bind.buffer = nullptr;
  bind.buffer_length = 0;
  bind.length = &output_string.output_length;
}

void BindsStorage::DoBind(std::size_t pos, enum_field_types type, void* buffer,
                          std::size_t length, bool is_unsigned) {
  ResizeIfNecessary(pos);

  auto& bind = binds_[pos];
  bind.buffer_type = type;
  bind.buffer = buffer;
  bind.buffer_length = length;
  bind.is_unsigned = is_unsigned;

  if (binds_type_ == BindsType::kResult) {
    UpdateBindBufferLength(bind, type);
  }
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
