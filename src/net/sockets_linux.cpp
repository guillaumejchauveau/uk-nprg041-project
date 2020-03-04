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

std::unique_ptr<Socket> Socket::accept(bool non_blocking_accepted) const {
  this->checkState();
  socklen_t client_sock_address_len = sizeof(sockaddr_storage);
  auto client_sock_address =
    reinterpret_cast<sockaddr *>(new sockaddr_storage);
  memset(client_sock_address, 0, static_cast<size_t>(client_sock_address_len));

  int flags = 0;
  if (non_blocking_accepted) {
    flags = SOCK_NONBLOCK;
  }
  socket_handle_t client_socket =
    ::accept4(this->handle_, client_sock_address, &client_sock_address_len, flags);
  if (client_socket == INVALID_SOCKET_HANDLE) {
    auto error = utils::Exception::getLastFailureCode();
    if (isCodeEWouldBlock(error)) {
      return nullptr;
    }
    throw utils::Exception(error);
  }
  auto socket_address = std::make_unique<SocketAddress>(client_sock_address,
                                                        client_sock_address_len);
  delete client_sock_address;
  return std::make_unique<Socket>(client_socket, std::move(socket_address));
}

void Socket::close() {
  ::close(this->handle_);
  this->handle_ = INVALID_SOCKET_HANDLE;
}

SocketInitializer::SocketInitializer() = default;
SocketInitializer::~SocketInitializer() = default;
} // namespace net