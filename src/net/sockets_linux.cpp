#include "sockets.h"
#include <unistd.h>
#include <fcntl.h>

namespace net {
bool Socket::isCodeEWouldBlock(long int code) {
  return code == EWOULDBLOCK || code == EAGAIN;
}

bool Socket::isCodeEInProgress(long int code) {
  return code == EINPROGRESS;
}

void Socket::setNonBlocking() {
  int flags = fcntl(this->handle_, F_GETFL, 0);
  if (flags < 0) {
    throw utils::Exception::fromLastFailure();
  }
  if (fcntl(this->handle_, F_SETFL, flags | O_NONBLOCK) != 0) {
    throw utils::Exception::fromLastFailure();
  }
}

socket_handle_t Socket::acceptCall(sockaddr *client_sock_address,
                                   socklen_t *client_sock_address_len,
                                   bool non_blocking_accepted) const {
  int flags = 0;
  if (non_blocking_accepted) {
    flags = SOCK_NONBLOCK;
  }
  return
    ::accept4(this->handle_, client_sock_address, client_sock_address_len, flags);
}

void Socket::close() {
  ::close(this->handle_);
  this->handle_ = INVALID_SOCKET_HANDLE;
}

SocketInitializer::SocketInitializer() = default;
SocketInitializer::~SocketInitializer() = default;
} // namespace net
