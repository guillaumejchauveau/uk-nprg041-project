#include "tcp.h"

namespace net {
TCPServer::TCPServer(std::unique_ptr<Socket> &&socket) : socket_(std::move(socket)) {

}

TCPServer::TCPServer(TCPServer &&tcp_server) noexcept : socket_(std::move(tcp_server.socket_)) {

}

TCPServer &TCPServer::operator=(TCPServer &&tcp_server) noexcept {
  if (this == &tcp_server) {
    return *this;
  }
  this->socket_ = std::move(tcp_server.socket_);
  return *this;
}

void TCPServer::listen(int max) {

}

void TCPServer::run() {

}
} // namespace net
