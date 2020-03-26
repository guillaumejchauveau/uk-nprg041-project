#include "sockets.h"
#include <unistd.h>
#include <fcntl.h>

namespace net {
bool Socket::isErrorEWouldBlock(long int error) {
  return error == EWOULDBLOCK || error == EAGAIN;
}

bool Socket::isErrorEInProgress(long int error) {
  return error == EINPROGRESS;
}

void Socket::setNonBlocking() {
  int flags = fcntl(this->handle_, F_GETFL, 0);
  if (flags < 0) {
    throw utils::SystemException::fromLastError();
  }
  if (fcntl(this->handle_, F_SETFL, flags | O_NONBLOCK) != 0) {
    throw utils::SystemException::fromLastError();
  }
}

unique_ptr<Socket> Socket::accept(bool set_non_blocking) const {
  this->checkState();
  socklen_t client_sock_address_len = sizeof(sockaddr_storage);
  auto client_sock_address =
    reinterpret_cast<sockaddr *>(new sockaddr_storage);
  memset(client_sock_address, 0, static_cast<size_t>(client_sock_address_len));

  int flags = 0;
  if (set_non_blocking) {
    flags = SOCK_NONBLOCK;
  }
  socket_handle_t client_socket =
    ::accept4(this->handle_, client_sock_address, &client_sock_address_len, flags);
  if (client_socket == INVALID_SOCKET_HANDLE) {
    auto error = utils::SystemException::getLastError();
    if (Socket::isErrorEWouldBlock(error)) {
      return nullptr;
    }
    throw utils::SystemException(error);
  }
  auto socket_address = make_unique<SocketAddress>(client_sock_address,
                                                   client_sock_address_len);
  delete client_sock_address;
  return make_unique<Socket>(client_socket, move(socket_address));
}

void Socket::close() {
  ::close(this->handle_);
  this->handle_ = INVALID_SOCKET_HANDLE;
}

SocketInitializer::SocketInitializer() = default;
SocketInitializer::~SocketInitializer() = default;
} // namespace net
