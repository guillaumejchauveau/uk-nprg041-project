#include "sockets.h"

#pragma comment(lib, "Ws2_32.lib")

namespace net {
void SocketException::throwFromFailure(int result, const char *tag) {
  char *wsa_message;
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr, WSAGetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                reinterpret_cast<LPSTR>(&wsa_message), 0, nullptr);
  auto message = std::string(wsa_message);
  LocalFree(wsa_message);
  throw SocketException("%s: %s", result, tag, message);
}

ssize_t Socket::recv(void *buf, size_t len, int flags) {
  auto result =
    ::recv(this->handle_, static_cast<char *>(buf), len, flags);
  if (result < 0) {
    SocketException::throwFromFailure(result, "RECVFROM");
  }
  return result;
}

ssize_t Socket::recvfrom(void *buf, size_t len, int flags,
                         SocketAddress &address) {
  auto result = ::recvfrom(this->handle_, static_cast<char *>(buf), len,
                           flags, &address.ai_addr, &address.ai_addrlen);
  if (result < 0) {
    SocketException::throwFromFailure(result, "RECVFROM");
  }
  return result;
}

ssize_t Socket::send(const void *buf, size_t len, int flags) {
  auto result =
    ::send(this->handle_, static_cast<const char *>(buf), len, flags);
  if (result < 0) {
    SocketException::throwFromFailure(result, "SEND");
  }
  return result;
}

ssize_t Socket::sendto(const void *buf, size_t len, int flags,
                       const SocketAddress &address) {
  auto result = ::sendto(this->handle_, static_cast<const char *>(buf), len,
                         flags, &address.ai_addr, address.ai_addrlen);
  if (result < 0) {
    SocketException::throwFromFailure(result, "SENDTO");
  }
  return result;
}

void Socket::close() {
  ::closesocket(this->handle_);
  this->handle_ = INVALID_SOCKET_HANDLE;
}
} // namespace net
