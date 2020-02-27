#include "sockets.h"
#include <unistd.h>

namespace net {
void Socket::close() {
  ::close(this->handle_);
  this->handle_ = INVALID_SOCKET_HANDLE;
}

SocketInitializer::SocketInitializer() = default;
SocketInitializer::~SocketInitializer() = default;
} // namespace net
