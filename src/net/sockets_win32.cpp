#include "sockets.h"

#pragma comment(lib, "Ws2_32.lib")

namespace net {
bool Socket::isCodeEWouldBlock(long int code) {
  return code == WSAEWOULDBLOCK;
}

bool Socket::isCodeEInProgress(long int code) {
  return code == WSAEWOULDBLOCK;
}

void Socket::close() {
  ::closesocket(this->handle_);
  this->handle_ = INVALID_SOCKET_HANDLE;
}

void Socket::setNonBlocking() {
  u_long mode = 1;
  if (ioctlsocket(this->handle_, FIONBIO, &mode) != 0) {
    throw utils::Exception::fromLastFailure();
  }
}

socket_handle_t Socket::acceptCall(sockaddr *client_sock_address,
                                   socklen_t *client_sock_address_len,
                                   bool non_blocking_accepted) {
  socket_handle_t client =
    ::accept(this->handle_, client_sock_address, client_sock_address_len);
  if (non_blocking_accepted && client != INVALID_SOCKET_HANDLE) {
    this->setNonBlocking();
  }
  return client;
}


SocketInitializer::SocketInitializer() {
  if (WSAStartup(MAKEWORD(2, 2), &this->wsadata_) == SOCKET_ERROR) {
    throw utils::Exception::fromLastFailure();
  }
}

SocketInitializer::~SocketInitializer() {
  WSACleanup();
}
} // namespace net
