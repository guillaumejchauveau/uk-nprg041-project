#include <fcntl.h>
#include "tcp.h"

namespace net {

TCPServer::TCPServer(std::unique_ptr<Socket> &&socket) : socket_(std::move(socket)) {
  int flags = ::fcntl(this->socket_->getHandle(), F_GETFL, 0);
  if (flags != 0) {

  }
  if (::fcntl(this->socket_->getHandle(), F_SETFL, flags & ~O_NONBLOCK) != 0) {

  }
}
} // namespace net
