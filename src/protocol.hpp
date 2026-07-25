#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>

#include "binary_codec.hpp"
#include "runtime.hpp"

namespace minesweeper {

constexpr std::uint16_t kMinesweeperProtocolVersion = 2;

enum class WireErrorCode : std::uint8_t {
  BadRequest = 1,
  GameNotFound = 2,
  InvalidAction = 3,
  SystemUnavailable = 4,
};

class RequestDecodeError : public std::invalid_argument {
public:
  RequestDecodeError(std::string request_id, const std::string& message)
      : std::invalid_argument(message), request_id_(std::move(request_id)) {}

  const std::string& request_id() const {
    return request_id_;
  }

private:
  std::string request_id_;
};

struct RequestEnvelope {
  std::string request_id;
  std::string game_id;
  Command command;
};

RequestEnvelope decode_request(const Bytes& payload);
Bytes encode_result(
    const std::string& request_id,
    const std::string& game_id,
    const Result& result);
Bytes encode_error(
    const std::string& request_id,
    WireErrorCode code,
    const std::string& message);

}  // namespace minesweeper
