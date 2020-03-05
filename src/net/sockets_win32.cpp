#include "sockets.h"

#pragma comment(lib, "Ws2_32.lib")

namespace net {
bool Socket::isErrorEWouldBlock(long int error) {
  return error == WSAEWOULDBLOCK;
}

bool Socket::isErrorEInProgress(long int error) {
  return error == WSAEWOULDBLOCK;
}

void Socket::close() {
  ::closesocket(this->handle_);
  this->handle_ = INVALID_SOCKET_HANDLE;
}

void Socket::setNonBlocking() {
  u_long mode = 1;
  if (ioctlsocket(this->handle_, FIONBIO, &mode) != 0) {
    throw utils::Exception::fromLastError();
  }
}

std::unique_ptr<Socket> Socket::accept(bool non_blocking_accepted) const {
  this->checkState();
  socklen_t client_sock_address_len = sizeof(sockaddr_storage);
  auto client_sock_address =
    reinterpret_cast<sockaddr *>(new sockaddr_storage);
  memset(client_sock_address, 0, static_cast<size_t>(client_sock_address_len));

  socket_handle_t client_socket =
    ::accept(this->handle_, client_sock_address, &client_sock_address_len);
  if (client_socket == INVALID_SOCKET_HANDLE) {
    auto error = utils::Exception::getLastError();
    if (Socket::isErrorEWouldBlock(error)) {
      return nullptr;
    }
    throw utils::Exception(error);
  }
  auto socket_address = std::make_unique<SocketAddress>(client_sock_address,
                                                        client_sock_address_len);
  delete client_sock_address;
  auto client = std::make_unique<Socket>(client_socket, std::move(socket_address));
  if (non_blocking_accepted) {
    client->setNonBlocking();
  }
  return client;
}

SocketInitializer::SocketInitializer() {
  if (WSAStartup(MAKEWORD(2, 2), &this->wsadata_) == SOCKET_ERROR) {
    throw utils::Exception::fromLastError();
  }
}

SocketInitializer::~SocketInitializer() {
  WSACleanup();
}
} // namespace net
