#include "sockets.h"

#pragma comment(lib, "Ws2_32.lib")

namespace net {
void SocketException::throwFromFailure(result_t result, const char *tag) {
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

void Socket::close() {
  ::closesocket(this->handle_);
  this->handle_ = INVALID_SOCKET_HANDLE;
}

WSADATA wsadata;

SocketInitializer::SocketInitializer() {
  if (WSAStartup(MAKEWORD(2, 2), &wsadata) == SOCKET_ERROR) {
    throw std::exception();
  }
}

SocketInitializer::~SocketInitializer() {
  WSACleanup();
}
} // namespace net
