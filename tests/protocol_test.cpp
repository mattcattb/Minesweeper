#include <cassert>
#include <stdexcept>
#include <variant>

#include "protocol.hpp"

int main() {
  using namespace minesweeper;

  const Bytes payload{
      0, 2,
      0, 1, 'r',
      1,
      0, 4, 'g', 'a', 'm', 'e',
      0, 4,
      0, 5,
      0, 3,
  };
  const RequestEnvelope request = decode_request(payload);
  assert(request.request_id == "r");
  assert(request.game_id == "game");
  const CreateGame& command = std::get<CreateGame>(request.command);
  assert(command.rows == 4);
  assert(command.cols == 5);
  assert(command.mines == 3);

  Bytes invalid = payload;
  invalid.push_back(0);
  bool rejected_trailing_data = false;
  try {
    static_cast<void>(decode_request(invalid));
  } catch (const std::invalid_argument&) {
    rejected_trailing_data = true;
  }
  assert(rejected_trailing_data);

  Bytes unsupported = payload;
  unsupported[1] = 99;
  bool preserved_request_id = false;
  try {
    static_cast<void>(decode_request(unsupported));
  } catch (const RequestDecodeError& error) {
    preserved_request_id = error.request_id() == "r";
  }
  assert(preserved_request_id);

  Bytes invalid_utf8 = payload;
  invalid_utf8[8] = 0xff;
  bool rejected_invalid_utf8 = false;
  try {
    static_cast<void>(decode_request(invalid_utf8));
  } catch (const RequestDecodeError&) {
    rejected_invalid_utf8 = true;
  }
  assert(rejected_invalid_utf8);

  const GameSnapshot snapshot{
      0,
      GameStatus::Playing,
      0,
      1,
      1,
      1,
      {HiddenTileView{0, 0, false}},
  };
  const Bytes encoded_snapshot = encode_result(
      "request", "game", Result{EventAudience::Connection, snapshot});
  const Bytes expected_snapshot{
      0, 2,
      1,
      0, 7, 'r', 'e', 'q', 'u', 'e', 's', 't',
      2,
      0, 4, 'g', 'a', 'm', 'e',
      0, 0, 0, 0, 0, 0, 0, 0,
      1,
      0, 0, 0, 0,
      0, 0, 0, 1,
      0, 1,
      0, 1,
      1, 0,
  };
  assert(encoded_snapshot == expected_snapshot);

  const Bytes encoded_error = encode_error(
      "r", WireErrorCode::InvalidAction, "blocked");
  const Bytes expected_error{
      0, 2,
      2,
      0, 1, 'r',
      3,
      0, 7, 'b', 'l', 'o', 'c', 'k', 'e', 'd',
  };
  assert(encoded_error == expected_error);
}
