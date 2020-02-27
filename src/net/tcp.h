#ifndef NET_TCP_H
#define NET_TCP_H

#include "sockets.h"
#include "../utils/exception.h"

namespace net {

class TCPServer {
protected:
  std::unique_ptr<Socket> socket_;

public:
  TCPServer(std::unique_ptr<Socket> &&socket);

  TCPServer(const TCPServer &tcp_server) = delete;

  TCPServer(TCPServer &&tcp_server) noexcept : socket_(std::move(tcp_server.socket_)) {

  }

  TCPServer operator=(const TCPServer &tcp_server) = delete;

  TCPServer &operator=(TCPServer &&tcp_server) noexcept {
    if (this == &tcp_server) {
      return *this;
    }
    this->socket_ = std::move(tcp_server.socket_);
    return *this;
  }

  template<typename ... Args>
  static std::unique_ptr<TCPServer> fromFactory(Args ... args) {
    return std::make_unique<TCPServer>(SocketFactory::boundSocket(args...));
  }
};

} // namespace net

#endif //NET_TCP_H
