#include "sockets.h"
#include <unistd.h>

namespace net {
void SocketException::throwFromFailure(int result, const char *tag) {
  throw SocketException("%s: %s", result, tag, strerror(errno));
}

ssize_t Socket::recv(void *buf, size_t len, int flags) {
  auto result = ::recv(this->handle_, buf, len, flags);
  if (result < 0) {
    SocketException::throwFromFailure(result, "RECV");
  }
  return result;
}

ssize_t Socket::recvfrom(void *buf, size_t len, int flags,
                         SocketAddress &address) {
  auto result = ::recvfrom(this->handle_, buf, len, flags, &address.ai_addr,
                           &address.ai_addrlen);
  if (result < 0) {
    SocketException::throwFromFailure(result, "RECVFROM");
  }
  return result;
}

ssize_t Socket::send(const void *buf, size_t len, int flags) {
  auto result = ::send(this->handle_, buf, len, flags);
  if (result < 0) {
    SocketException::throwFromFailure(result, "SEND");
  }
  return result;
}

ssize_t Socket::sendto(const void *buf, size_t len, int flags,
                       const SocketAddress &address) {
  auto result = ::sendto(this->handle_, buf, len, flags, &address.ai_addr,
                         address.ai_addrlen);
  if (result < 0) {
    SocketException::throwFromFailure(result, "SENDTO");
  }
  return result;
}

void Socket::close() {
  ::close(this->handle_);
  this->handle_ = INVALID_SOCKET_HANDLE;
}
} // namespace net
