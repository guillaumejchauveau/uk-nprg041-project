#include "tcp.h"

namespace net {
TCPServer::TCPServer(unique_ptr<Socket> &&socket) : socket_(move(socket)) {

}

TCPServer::TCPServer(TCPServer &&tcp_server) noexcept : socket_(move(tcp_server.socket_)) {

}

TCPServer &TCPServer::operator=(TCPServer &&tcp_server) noexcept {
  if (this == &tcp_server) {
    return *this;
  }
  this->socket_ = move(tcp_server.socket_);
  return *this;
}

void TCPServer::initialize(int max) {

}

void TCPServer::run() {

}
} // namespace net
