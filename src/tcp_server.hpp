#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>

#include "binary_codec.hpp"
#include "protocol.hpp"
#include "runtime.hpp"

namespace minesweeper {

class TcpServer {
public:
  TcpServer(std::uint16_t port, GameRuntime& runtime)
      : port_(port), runtime_(runtime) {}
  ~TcpServer();

  bool run();

private:
  struct Connection {
    Bytes input;
    std::deque<std::string> output;
    std::size_t output_offset = 0;
  };

  bool open_listener();
  void accept_connections();
  bool read_connection(int socket);
  bool process_frames(int socket);
  void process_payload(int socket, const Bytes& payload);
  bool write_connection(int socket);
  void queue_result(
      int socket,
      const std::string& request_id,
      const std::string& game_id,
      const Result& result);
  void queue_error(
      int socket,
      const std::string& request_id,
      WireErrorCode code,
      const std::string& message);
  void queue_message(int socket, const Bytes& payload);
  void close_connection(int socket);

  std::uint16_t port_;
  int listener_ = -1;
  GameRuntime& runtime_;
  std::unordered_map<int, Connection> connections_;
};

}  // namespace minesweeper
