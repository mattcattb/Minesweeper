#include "tcp_server.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <limits>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <utility>
#include <vector>

#include "protocol.hpp"

namespace minesweeper {

namespace {

constexpr std::size_t kFrameHeaderSize = 4;
constexpr std::size_t kMaximumFrameSize = 4 * 1024 * 1024;

bool make_nonblocking(int socket) {
  const int flags = fcntl(socket, F_GETFL, 0);
  return flags >= 0 && fcntl(socket, F_SETFL, flags | O_NONBLOCK) == 0;
}

}  // namespace

TcpServer::~TcpServer() {
  for (const auto& [socket, connection] : connections_) {
    static_cast<void>(connection);
    close(socket);
  }
  if (listener_ >= 0) {
    close(listener_);
  }
}

bool TcpServer::open_listener() {
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

bool TcpServer::run() {
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
          ((descriptor.revents & POLLIN) != 0 &&
              !read_connection(descriptor.fd)) ||
          ((descriptor.revents & POLLOUT) != 0 &&
              !write_connection(descriptor.fd))) {
        closed.push_back(descriptor.fd);
      }
    }
    for (int socket : closed) {
      close_connection(socket);
    }
  }
}

void TcpServer::accept_connections() {
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

bool TcpServer::read_connection(int socket) {
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

bool TcpServer::process_frames(int socket) {
  Bytes& input = connections_.at(socket).input;
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

    const Bytes payload(
        input.begin() + kFrameHeaderSize,
        input.begin() + kFrameHeaderSize + frame_size);
    input.erase(input.begin(), input.begin() + kFrameHeaderSize + frame_size);
    process_payload(socket, payload);
  }
  return true;
}

void TcpServer::process_payload(int socket, const Bytes& payload) {
  RequestEnvelope request;
  try {
    request = decode_request(payload);
  } catch (const RequestDecodeError& error) {
    queue_error(
        socket, error.request_id(), WireErrorCode::BadRequest, error.what());
    return;
  } catch (const std::exception& error) {
    queue_error(socket, "", WireErrorCode::BadRequest, error.what());
    return;
  }

  try {
    queue_result(socket, request.request_id, request.game_id,
        runtime_.execute(request.game_id, request.command));
  } catch (const std::exception& error) {
    queue_error(socket, request.request_id,
        WireErrorCode::SystemUnavailable, error.what());
  }
}

void TcpServer::queue_result(
    int socket,
    const std::string& request_id,
    const std::string& game_id,
    const Result& result) {
  queue_message(socket, encode_result(request_id, game_id, result));
}

void TcpServer::queue_error(
    int socket,
    const std::string& request_id,
    WireErrorCode code,
    const std::string& message) {
  queue_message(socket, encode_error(request_id, code, message));
}

void TcpServer::queue_message(int socket, const Bytes& payload) {
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
  frame.append(
      reinterpret_cast<const char*>(payload.data()), payload.size());
  found->second.output.push_back(std::move(frame));
}

bool TcpServer::write_connection(int socket) {
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

void TcpServer::close_connection(int socket) {
  close(socket);
  connections_.erase(socket);
}

}  // namespace minesweeper
