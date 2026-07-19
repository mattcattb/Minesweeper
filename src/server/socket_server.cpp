#include "server/socket_server.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <limits>
#include <optional>
#include <utility>

namespace {

using json = nlohmann::json;

constexpr std::size_t kFrameHeaderSize = 4;
constexpr std::size_t kMaximumFrameSize = 4 * 1024 * 1024;

bool make_nonblocking(int socket) {
  const int flags = fcntl(socket, F_GETFL, 0);
  return flags >= 0 && fcntl(socket, F_SETFL, flags | O_NONBLOCK) == 0;
}

std::optional<std::string> required_string(
    const json& message,
    const char* key) {
  const auto value = message.find(key);
  if (value == message.end() || !value->is_string() || value->empty()) {
    return std::nullopt;
  }
  return value->get<std::string>();
}

std::string game_status_name(GameStatus status) {
  switch (status) {
    case GameStatus::Playing:
      return "playing";
    case GameStatus::Won:
      return "won";
    case GameStatus::Lost:
      return "lost";
  }
  return "playing";
}

std::string error_code_name(CommandErrorCode code) {
  switch (code) {
    case CommandErrorCode::BadRequest:
      return "BAD_REQUEST";
    case CommandErrorCode::GameNotFound:
      return "GAME_NOT_FOUND";
    case CommandErrorCode::InvalidAction:
      return "INVALID_ACTION";
  }
  return "INVALID_ACTION";
}

json serialize_event(const GameSnapshot& snapshot) {
  json tiles = json::array();
  for (const TileView& tile : snapshot.tiles) {
    if (const auto* hidden = std::get_if<HiddenTileView>(&tile)) {
      tiles.push_back({
          {"row", hidden->row},
          {"col", hidden->col},
          {"state", "hidden"},
          {"flagged", hidden->flagged},
      });
      continue;
    }

    const RevealedTileView& revealed = std::get<RevealedTileView>(tile);
    tiles.push_back({
        {"row", revealed.row},
        {"col", revealed.col},
        {"state", "revealed"},
        {"value", revealed.mine ? json("mine") : json(revealed.adjacent_mines)},
    });
  }

  return {
      {"type", "game.snapshot"},
      {"audience",
          snapshot.audience == EventAudience::Game ? "game" : "connection"},
      {"gameId", snapshot.game_id},
      {"connectionId", snapshot.connection_id},
      {"requestId", snapshot.request_id},
      {"payload", {
          {"revision", snapshot.revision},
          {"status", game_status_name(snapshot.status)},
          {"elapsedSeconds", snapshot.elapsed_seconds},
          {"remainingMines", snapshot.remaining_mines},
          {"rows", snapshot.rows},
          {"cols", snapshot.cols},
          {"tiles", std::move(tiles)},
      }},
  };
}

json serialize_event(const CommandRejected& rejected) {
  return {
      {"type", "error"},
      {"connectionId", rejected.connection_id},
      {"requestId", rejected.request_id},
      {"payload", {
          {"code", error_code_name(rejected.code)},
          {"message", rejected.message},
      }},
  };
}

}  // namespace

SocketServer::~SocketServer() {
  for (const auto& [socket, connection] : connections_) {
    static_cast<void>(connection);
    close(socket);
  }
  if (listener_ >= 0) {
    close(listener_);
  }
}

bool SocketServer::open_listener() {
  listener_ = socket(AF_INET, SOCK_STREAM, 0);
  if (listener_ < 0) {
    std::cerr << "Unable to create Minesweeper socket: "
              << std::strerror(errno) << '\n';
    return false;
  }

  const int reuse_address = 1;
  setsockopt(listener_, SOL_SOCKET, SO_REUSEADDR,
      &reuse_address, sizeof(reuse_address));

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(port_);
  if (bind(listener_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0 ||
      listen(listener_, SOMAXCONN) < 0 ||
      !make_nonblocking(listener_)) {
    std::cerr << "Unable to listen on Minesweeper port " << port_ << ": "
              << std::strerror(errno) << '\n';
    return false;
  }

  return true;
}

bool SocketServer::run() {
  if (!open_listener()) {
    return false;
  }

  std::cout << "Minesweeper socket server listening on 0.0.0.0:"
            << port_ << '\n';
  while (true) {
    std::vector<pollfd> descriptors;
    descriptors.reserve(connections_.size() + 1);
    descriptors.push_back({listener_, POLLIN, 0});
    for (const auto& [socket, connection] : connections_) {
      short events = POLLIN;
      if (!connection.output.empty()) {
        events = static_cast<short>(events | POLLOUT);
      }
      descriptors.push_back({socket, events, 0});
    }

    if (poll(descriptors.data(), descriptors.size(), -1) < 0) {
      if (errno == EINTR) {
        continue;
      }
      std::cerr << "Minesweeper poll failed: " << std::strerror(errno) << '\n';
      return false;
    }

    if ((descriptors.front().revents & POLLIN) != 0) {
      accept_connections();
    }

    std::vector<int> closed;
    for (std::size_t index = 1; index < descriptors.size(); index += 1) {
      const pollfd descriptor = descriptors[index];
      if (connections_.find(descriptor.fd) == connections_.end()) {
        continue;
      }
      if ((descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0 ||
          ((descriptor.revents & POLLIN) != 0 && !read_connection(descriptor.fd)) ||
          ((descriptor.revents & POLLOUT) != 0 && !write_connection(descriptor.fd))) {
        closed.push_back(descriptor.fd);
      }
    }
    for (int socket : closed) {
      close_connection(socket);
    }
  }
}

void SocketServer::accept_connections() {
  while (true) {
    const int client = accept(listener_, nullptr, nullptr);
    if (client < 0) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        std::cerr << "Unable to accept Minesweeper connection: "
                  << std::strerror(errno) << '\n';
      }
      return;
    }
    if (!make_nonblocking(client)) {
      close(client);
      continue;
    }
    connections_.emplace(client, Connection{});
  }
}

bool SocketServer::read_connection(int socket) {
  auto found = connections_.find(socket);
  if (found == connections_.end()) {
    return false;
  }

  std::uint8_t buffer[16 * 1024];
  while (true) {
    const ssize_t received = recv(socket, buffer, sizeof(buffer), 0);
    if (received > 0) {
      found->second.input.insert(
          found->second.input.end(), buffer, buffer + received);
      if (!process_frames(socket)) {
        return false;
      }
      continue;
    }
    if (received == 0) {
      return false;
    }
    return errno == EAGAIN || errno == EWOULDBLOCK;
  }
}

bool SocketServer::process_frames(int socket) {
  auto& input = connections_.at(socket).input;
  while (input.size() >= kFrameHeaderSize) {
    const std::uint32_t frame_size =
        (static_cast<std::uint32_t>(input[0]) << 24U) |
        (static_cast<std::uint32_t>(input[1]) << 16U) |
        (static_cast<std::uint32_t>(input[2]) << 8U) |
        static_cast<std::uint32_t>(input[3]);
    if (frame_size == 0 || frame_size > kMaximumFrameSize) {
      return false;
    }
    if (input.size() < kFrameHeaderSize + frame_size) {
      return true;
    }

    const std::string payload(
        input.begin() + kFrameHeaderSize,
        input.begin() + kFrameHeaderSize + frame_size);
    input.erase(input.begin(), input.begin() + kFrameHeaderSize + frame_size);
    process_message(socket, payload);
  }
  return true;
}

void SocketServer::process_message(int socket, const std::string& payload) {
  const json message = json::parse(payload, nullptr, false);
  if (message.is_discarded() || !message.is_object()) {
    queue_error(socket, "", "", "BAD_REQUEST", "Invalid JSON command");
    return;
  }

  const std::string connection_id =
      message.value("connectionId", std::string{});
  const std::string request_id = message.value("requestId", std::string{});
  const auto type = required_string(message, "type");
  const auto game_id = required_string(message, "gameId");
  if (!type || !game_id || request_id.empty()) {
    queue_error(socket, connection_id, request_id, "BAD_REQUEST",
        "Command requires type, gameId, and requestId");
    return;
  }

  try {
    if (*type == "game.create") {
      const json& data = message.at("payload");
      server_.enqueue(CreateGame{
          *game_id,
          request_id,
          data.at("rows").get<int>(),
          data.at("cols").get<int>(),
          data.at("mines").get<int>(),
      });
    } else if (*type == "tile.reveal" || *type == "tile.flag.toggle") {
      if (connection_id.empty()) {
        throw std::invalid_argument("Tile command requires connectionId");
      }
      const json& data = message.at("payload");
      const int row = data.at("row").get<int>();
      const int col = data.at("col").get<int>();
      if (*type == "tile.reveal") {
        server_.enqueue(RevealTile{
            connection_id, *game_id, request_id, row, col});
      } else {
        server_.enqueue(ToggleFlag{
            connection_id, *game_id, request_id, row, col});
      }
    } else if (*type == "game.restart") {
      server_.enqueue(RestartGame{connection_id, *game_id, request_id});
    } else if (*type == "game.resync") {
      if (connection_id.empty()) {
        throw std::invalid_argument("Resync requires connectionId");
      }
      server_.enqueue(RequestSnapshot{connection_id, *game_id, request_id});
    } else {
      queue_error(socket, connection_id, request_id, "BAD_REQUEST",
          "Unknown Minesweeper command type");
      return;
    }
  } catch (const std::exception& error) {
    queue_error(socket, connection_id, request_id, "BAD_REQUEST", error.what());
    return;
  }

  if (server_.process_one()) {
    publish_events(socket);
  }
}

void SocketServer::publish_events(int source_socket) {
  ServerEvent event;
  while (server_.try_pop_event(event)) {
    const std::string payload = std::visit(
        [](const auto& value) { return serialize_event(value).dump(); }, event);
    const auto* snapshot = std::get_if<GameSnapshot>(&event);
    if (snapshot != nullptr && snapshot->audience == EventAudience::Game) {
      for (const auto& [socket, connection] : connections_) {
        static_cast<void>(connection);
        queue_message(socket, payload);
      }
    } else {
      queue_message(source_socket, payload);
    }
  }
}

void SocketServer::queue_error(
    int socket,
    const std::string& connection_id,
    const std::string& request_id,
    const std::string& code,
    const std::string& message) {
  queue_message(socket, json{
      {"type", "error"},
      {"connectionId", connection_id},
      {"requestId", request_id},
      {"payload", {{"code", code}, {"message", message}}},
  }.dump());
}

void SocketServer::queue_message(int socket, const std::string& payload) {
  const auto found = connections_.find(socket);
  if (found == connections_.end() ||
      payload.size() > std::numeric_limits<std::uint32_t>::max()) {
    return;
  }

  const std::uint32_t size = static_cast<std::uint32_t>(payload.size());
  std::string frame(kFrameHeaderSize, '\0');
  frame[0] = static_cast<char>((size >> 24U) & 0xffU);
  frame[1] = static_cast<char>((size >> 16U) & 0xffU);
  frame[2] = static_cast<char>((size >> 8U) & 0xffU);
  frame[3] = static_cast<char>(size & 0xffU);
  frame += payload;
  found->second.output.push_back(std::move(frame));
}

bool SocketServer::write_connection(int socket) {
  Connection& connection = connections_.at(socket);
  while (!connection.output.empty()) {
    const std::string& frame = connection.output.front();
    const char* data = frame.data() + connection.output_offset;
    const std::size_t remaining = frame.size() - connection.output_offset;
    const ssize_t sent = send(socket, data, remaining, 0);
    if (sent > 0) {
      connection.output_offset += static_cast<std::size_t>(sent);
      if (connection.output_offset == frame.size()) {
        connection.output.pop_front();
        connection.output_offset = 0;
      }
      continue;
    }
    if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      return true;
    }
    return false;
  }
  return true;
}

void SocketServer::close_connection(int socket) {
  close(socket);
  connections_.erase(socket);
}
