#include "protocol.hpp"

#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace minesweeper {

namespace {

enum class CommandKind : std::uint8_t {
  CreateGame = 1,
  RevealTile = 2,
  ToggleFlag = 3,
  RestartGame = 4,
  RequestSnapshot = 5,
};

enum class EventKind : std::uint8_t {
  Snapshot = 1,
  Error = 2,
};

enum class AudienceKind : std::uint8_t {
  Game = 1,
  Connection = 2,
};

enum class StatusKind : std::uint8_t {
  Playing = 1,
  Won = 2,
  Lost = 3,
};

enum class TileKind : std::uint8_t {
  Hidden = 1,
  Revealed = 2,
};

Command decode_command(Decoder& decoder, CommandKind kind) {
  switch (kind) {
    case CommandKind::CreateGame:
      return CreateGame{
          decoder.read_u16(), decoder.read_u16(), decoder.read_u16()};
    case CommandKind::RevealTile:
      return RevealTile{{decoder.read_u16(), decoder.read_u16()}};
    case CommandKind::ToggleFlag:
      return ToggleFlag{{decoder.read_u16(), decoder.read_u16()}};
    case CommandKind::RestartGame:
      return RestartGame{};
    case CommandKind::RequestSnapshot:
      return RequestSnapshot{};
  }
  throw std::invalid_argument("Unknown Minesweeper command kind");
}

template <typename Target, typename Source>
Target checked_integer(Source value, const char* field) {
  static_assert(std::is_integral_v<Target>);
  static_assert(std::is_integral_v<Source>);
  if (value < 0 ||
      static_cast<std::uint64_t>(value) >
          static_cast<std::uint64_t>(std::numeric_limits<Target>::max())) {
    throw std::invalid_argument(
        std::string(field) + " exceeds the Minesweeper protocol limit");
  }
  return static_cast<Target>(value);
}

StatusKind encode_status(minesweeper::GameStatus status) {
  switch (status) {
    case minesweeper::GameStatus::Playing:
      return StatusKind::Playing;
    case minesweeper::GameStatus::Won:
      return StatusKind::Won;
    case minesweeper::GameStatus::Lost:
      return StatusKind::Lost;
  }
  throw std::invalid_argument("Unknown Minesweeper game status");
}

WireErrorCode encode_error_code(CommandErrorCode code) {
  switch (code) {
    case CommandErrorCode::BadRequest:
      return WireErrorCode::BadRequest;
    case CommandErrorCode::GameNotFound:
      return WireErrorCode::GameNotFound;
    case CommandErrorCode::InvalidAction:
      return WireErrorCode::InvalidAction;
  }
  throw std::invalid_argument("Unknown Minesweeper command error");
}

void encode_snapshot(Encoder& encoder, const GameSnapshot& snapshot) {
  const auto rows = checked_integer<std::uint16_t>(snapshot.rows, "Rows");
  const auto cols = checked_integer<std::uint16_t>(snapshot.cols, "Columns");
  const auto tile_count =
      static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols);
  if (snapshot.tiles.size() != tile_count) {
    throw std::invalid_argument(
        "Minesweeper snapshot tile count does not match its dimensions");
  }

  encoder.write_u64(snapshot.revision);
  encoder.write_u8(static_cast<std::uint8_t>(encode_status(snapshot.status)));
  encoder.write_u32(checked_integer<std::uint32_t>(
      snapshot.elapsed_seconds, "Elapsed seconds"));
  if (snapshot.remaining_mines < std::numeric_limits<std::int32_t>::min() ||
      snapshot.remaining_mines > std::numeric_limits<std::int32_t>::max()) {
    throw std::invalid_argument(
        "Remaining mines exceeds the Minesweeper protocol limit");
  }
  encoder.write_i32(static_cast<std::int32_t>(snapshot.remaining_mines));
  encoder.write_u16(rows);
  encoder.write_u16(cols);

  for (std::size_t index = 0; index < snapshot.tiles.size(); index += 1) {
    const int expected_row = static_cast<int>(index / cols);
    const int expected_col = static_cast<int>(index % cols);
    if (const auto* hidden = std::get_if<HiddenTileView>(
            &snapshot.tiles[index])) {
      if (hidden->row != expected_row || hidden->col != expected_col) {
        throw std::invalid_argument(
            "Minesweeper snapshot tiles are not row-major");
      }
      encoder.write_u8(static_cast<std::uint8_t>(TileKind::Hidden));
      encoder.write_u8(hidden->flagged ? 1 : 0);
      continue;
    }

    const auto& revealed = std::get<RevealedTileView>(snapshot.tiles[index]);
    if (revealed.row != expected_row || revealed.col != expected_col) {
      throw std::invalid_argument(
          "Minesweeper snapshot tiles are not row-major");
    }
    if (!revealed.mine &&
        (revealed.adjacent_mines < 0 || revealed.adjacent_mines > 8)) {
      throw std::invalid_argument(
          "Minesweeper adjacent mine count is outside 0 through 8");
    }
    encoder.write_u8(static_cast<std::uint8_t>(TileKind::Revealed));
    encoder.write_u8(
        revealed.mine ? 9 : static_cast<std::uint8_t>(
                                 revealed.adjacent_mines));
  }
}

}  // namespace

RequestEnvelope decode_request(const Bytes& payload) {
  Decoder decoder(payload);
  std::string request_id;
  try {
    const std::uint16_t version = decoder.read_u16();
    request_id = decoder.read_string();
    if (version != kMinesweeperProtocolVersion) {
      throw std::invalid_argument("Unsupported Minesweeper protocol version");
    }

    const auto kind = static_cast<CommandKind>(decoder.read_u8());
    std::string game_id = decoder.read_string();
    if (request_id.empty() || game_id.empty()) {
      throw std::invalid_argument("Request id and game id are required");
    }

    Command command = decode_command(decoder, kind);
    if (!decoder.finished()) {
      throw std::invalid_argument("Minesweeper request has trailing data");
    }
    return {std::move(request_id), std::move(game_id), std::move(command)};
  } catch (const RequestDecodeError&) {
    throw;
  } catch (const std::exception& error) {
    throw RequestDecodeError(std::move(request_id), error.what());
  }
}

Bytes encode_result(
    const std::string& request_id,
    const std::string& game_id,
    const Result& result) {
  if (const auto* error = std::get_if<CommandError>(&result.response)) {
    return encode_error(request_id, encode_error_code(error->code),
        error->message);
  }

  Encoder encoder;
  encoder.write_u16(kMinesweeperProtocolVersion);
  encoder.write_u8(static_cast<std::uint8_t>(EventKind::Snapshot));
  encoder.write_string(request_id);
  encoder.write_u8(static_cast<std::uint8_t>(
      result.audience == EventAudience::Game
          ? AudienceKind::Game
          : AudienceKind::Connection));
  encoder.write_string(game_id);
  encode_snapshot(encoder, std::get<GameSnapshot>(result.response));
  return encoder.finish();
}

Bytes encode_error(
    const std::string& request_id,
    WireErrorCode code,
    const std::string& message) {
  Encoder encoder;
  encoder.write_u16(kMinesweeperProtocolVersion);
  encoder.write_u8(static_cast<std::uint8_t>(EventKind::Error));
  encoder.write_string(request_id);
  encoder.write_u8(static_cast<std::uint8_t>(code));
  encoder.write_string(message);
  return encoder.finish();
}

}  // namespace minesweeper
