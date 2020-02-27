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

WSADATA wsadata;

SocketInitializer::SocketInitializer() {
  if (WSAStartup(MAKEWORD(2, 2), &wsadata) == SOCKET_ERROR) {
    throw utils::Exception::fromLastFailure();
  }
}

SocketInitializer::~SocketInitializer() {
  WSACleanup();
}
} // namespace net
