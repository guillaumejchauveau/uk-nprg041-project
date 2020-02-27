#include "sockets.h"
#include <unistd.h>

namespace net {
bool Socket::isCodeEWouldBlock(long int code) {
  return code == EWOULDBLOCK || code == EAGAIN;
}

bool Socket::isCodeEInProgress(long int code) {
  return code == EINPROGRESS;
}

void Socket::close() {
  ::close(this->handle_);
  this->handle_ = INVALID_SOCKET_HANDLE;
}

SocketInitializer::SocketInitializer() = default;
SocketInitializer::~SocketInitializer() = default;
} // namespace net
