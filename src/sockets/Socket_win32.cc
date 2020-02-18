#include "Socket.h"
#pragma comment(lib, "Ws2_32.lib")

namespace sockets {

void Socket::handleResult(int result, const std::string &tag) {
  if (result == SOCKET_ERROR) {
    char *wsa_message;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                  nullptr, WSAGetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  reinterpret_cast<LPSTR>(&wsa_message), 0, nullptr);
    auto message = tag + ": " + wsa_message;
    LocalFree(wsa_message);
    throw SocketException(message.data());
  }
}

ssize_t Socket::recv(void *buf, size_t len, int flags) {
  auto result =
      ::recv(this->handle_, reinterpret_cast<char *>(buf), len, flags);
  this->handleResult(result, "RECV");
  return result;
}
ssize_t Socket::recvfrom(void *buf, size_t len, int flags,
                         SocketAddress &address) {
  auto result = ::recvfrom(this->handle_, reinterpret_cast<char *>(buf), len,
                           flags, &address.ai_addr, &address.ai_addrlen);
  this->handleResult(result, "RECVFROM");
  return result;
}
ssize_t Socket::send(void *buf, size_t len, int flags) {
  auto result =
      ::send(this->handle_, reinterpret_cast<char *>(buf), len, flags);
  this->handleResult(result, "SEND");
  return result;
}
ssize_t Socket::sendto(void *buf, size_t len, int flags,
                       const SocketAddress &address) {
  auto result = ::sendto(this->handle_, reinterpret_cast<char *>(buf), len,
                         flags, &address.ai_addr, address.ai_addrlen);
  this->handleResult(result, "SENDTO");
  return result;
}

void Socket::close() { ::closesocket(this->handle_); }
} // namespace sockets
