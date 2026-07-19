#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

#include "server/server.h"

class SocketServer {
public:
  explicit SocketServer(std::uint16_t port) : port_(port) {}
  ~SocketServer();

  bool run();

private:
  struct Connection {
    std::vector<std::uint8_t> input;
    std::deque<std::string> output;
    std::size_t output_offset = 0;
  };

  bool open_listener();
  void accept_connections();
  bool read_connection(int socket);
  bool write_connection(int socket);
  bool process_frames(int socket);
  void process_message(int socket, const std::string& payload);
  void publish_events(int source_socket);
  void queue_error(
      int socket,
      const std::string& connection_id,
      const std::string& request_id,
      const std::string& code,
      const std::string& message);
  void queue_message(int socket, const std::string& payload);
  void close_connection(int socket);

  std::uint16_t port_;
  int listener_ = -1;
  Server server_;
  std::unordered_map<int, Connection> connections_;
};
