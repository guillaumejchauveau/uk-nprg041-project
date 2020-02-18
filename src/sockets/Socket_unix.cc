#include "Socket.h"
#include <cerrno>
#include <cstdlib>
#include <unistd.h>

namespace sockets {

void Socket::handleResult(int result, const std::string &tag) {
  if (result < 0) {
    auto message = tag + ": " + strerror(errno);
    throw SocketException(message.data());
  }
}

ssize_t Socket::recv(void *buf, size_t len, int flags) {
  auto result = ::recv(this->handle_, buf, len, flags);
  this->handleResult(result, "RECV");
  return result;
}

ssize_t Socket::recvfrom(void *buf, size_t len, int flags,
                         SocketAddress &address) {
  auto result = ::recvfrom(this->handle_, buf, len, flags, &address.ai_addr,
                           &address.ai_addrlen);
  this->handleResult(result, "RECVFROM");
  return result;
}

ssize_t Socket::send(void *buf, size_t len, int flags) {
  auto result = ::send(this->handle_, buf, len, flags);
  this->handleResult(result, "SEND");
  return result;
}
ssize_t Socket::sendto(void *buf, size_t len, int flags,
                       const SocketAddress &address) {
  auto result = ::sendto(this->handle_, buf, len, flags, &address.ai_addr,
                         address.ai_addrlen);
  this->handleResult(result, "SENDTO");
  return result;
}
void Socket::close() { ::close(this->handle_); }
} // namespace sockets
