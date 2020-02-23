#include "sockets.h"
#include <unistd.h>

namespace net {
void SocketException::throwFromFailure(result_t result, const char *tag) {
  throw SocketException("%s: %s", result, tag, strerror(errno));
}

void Socket::close() {
  ::close(this->handle_);
  this->handle_ = INVALID_SOCKET_HANDLE;
}

SocketInitializer::SocketInitializer() = default;
SocketInitializer::~SocketInitializer() = default;
} // namespace net
