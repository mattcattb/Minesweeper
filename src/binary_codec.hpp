#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace minesweeper {

using Bytes = std::vector<std::uint8_t>;

inline bool is_valid_utf8(const std::string& value) {
  std::size_t offset = 0;
  while (offset < value.size()) {
    const auto first = static_cast<std::uint8_t>(value[offset]);
    if (first <= 0x7fU) {
      offset += 1;
      continue;
    }

    std::size_t continuation_count = 0;
    std::uint32_t code_point = 0;
    std::uint32_t minimum = 0;
    if (first >= 0xc2U && first <= 0xdfU) {
      continuation_count = 1;
      code_point = first & 0x1fU;
      minimum = 0x80U;
    } else if (first >= 0xe0U && first <= 0xefU) {
      continuation_count = 2;
      code_point = first & 0x0fU;
      minimum = 0x800U;
    } else if (first >= 0xf0U && first <= 0xf4U) {
      continuation_count = 3;
      code_point = first & 0x07U;
      minimum = 0x10000U;
    } else {
      return false;
    }

    if (continuation_count >= value.size() - offset) {
      return false;
    }
    for (std::size_t index = 1; index <= continuation_count; index += 1) {
      const auto continuation =
          static_cast<std::uint8_t>(value[offset + index]);
      if ((continuation & 0xc0U) != 0x80U) {
        return false;
      }
      code_point = (code_point << 6U) | (continuation & 0x3fU);
    }
    if (code_point < minimum || code_point > 0x10ffffU ||
        (code_point >= 0xd800U && code_point <= 0xdfffU)) {
      return false;
    }
    offset += continuation_count + 1;
  }
  return true;
}

class Encoder {
public:
  void write_u8(std::uint8_t value) {
    bytes_.push_back(value);
  }

  void write_u16(std::uint16_t value) {
    bytes_.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
    bytes_.push_back(static_cast<std::uint8_t>(value & 0xffU));
  }

  void write_u32(std::uint32_t value) {
    for (int shift = 24; shift >= 0; shift -= 8) {
      bytes_.push_back(
          static_cast<std::uint8_t>((value >> shift) & 0xffU));
    }
  }

  void write_u64(std::uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8) {
      bytes_.push_back(
          static_cast<std::uint8_t>((value >> shift) & 0xffU));
    }
  }

  void write_i32(std::int32_t value) {
    write_u32(static_cast<std::uint32_t>(value));
  }

  void write_string(const std::string& value) {
    if (value.size() > std::numeric_limits<std::uint16_t>::max()) {
      throw std::length_error("String exceeds the Minesweeper protocol limit");
    }
    if (!is_valid_utf8(value)) {
      throw std::invalid_argument("String is not valid UTF-8");
    }
    write_u16(static_cast<std::uint16_t>(value.size()));
    bytes_.insert(bytes_.end(), value.begin(), value.end());
  }

  Bytes finish() {
    return std::move(bytes_);
  }

private:
  Bytes bytes_;
};

class Decoder {
public:
  explicit Decoder(const Bytes& bytes) : bytes_(bytes) {}

  std::uint8_t read_u8() {
    require(1);
    return bytes_[offset_++];
  }

  std::uint16_t read_u16() {
    require(2);
    const auto value = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(bytes_[offset_]) << 8U) |
        static_cast<std::uint16_t>(bytes_[offset_ + 1]));
    offset_ += 2;
    return value;
  }

  std::uint32_t read_u32() {
    require(4);
    std::uint32_t value = 0;
    for (int index = 0; index < 4; index += 1) {
      value = (value << 8U) | bytes_[offset_++];
    }
    return value;
  }

  std::uint64_t read_u64() {
    require(8);
    std::uint64_t value = 0;
    for (int index = 0; index < 8; index += 1) {
      value = (value << 8U) | bytes_[offset_++];
    }
    return value;
  }

  std::int32_t read_i32() {
    return static_cast<std::int32_t>(read_u32());
  }

  std::string read_string() {
    const std::size_t length = read_u16();
    require(length);
    const char* begin =
        reinterpret_cast<const char*>(bytes_.data() + offset_);
    std::string value(begin, length);
    offset_ += length;
    if (!is_valid_utf8(value)) {
      throw std::invalid_argument("Minesweeper string is not valid UTF-8");
    }
    return value;
  }

  bool finished() const {
    return offset_ == bytes_.size();
  }

  std::size_t remaining() const {
    return bytes_.size() - offset_;
  }

private:
  void require(std::size_t count) const {
    if (count > bytes_.size() - offset_) {
      throw std::invalid_argument("Truncated Minesweeper request");
    }
  }

  const Bytes& bytes_;
  std::size_t offset_ = 0;
};

}  // namespace minesweeper
