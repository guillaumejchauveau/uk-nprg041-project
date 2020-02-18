#include "sockets/Socket.h"
#include <cstring>
#include <exception>
#include <iostream>

int main() {
#if defined(_WIN32)
  WSADATA wsadata;
  if (WSAStartup(MAKEWORD(2, 2), &wsadata) == SOCKET_ERROR) {
    throw std::exception();
  }
#endif
  addrinfo hints{}, *result = nullptr;
  memset(&hints, 0, sizeof(addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;
  getaddrinfo(nullptr, "8081", &hints, &result);

  sockets::Socket *sock = nullptr;
  for (addrinfo *addr = result; addr != nullptr; addr = addr->ai_next) {
    try {
      sock = new sockets::Socket(addr);
    } catch (std::exception &e) {
      continue;
    }
    break;
  }
  freeaddrinfo(result);
  if (sock != nullptr) {
    sock->bind();
    sock->listen();
    auto client = sock->accept();
    char buf[100];
    memset(buf, 0, 100);
    client->recv(buf, 99);
    std::cout << buf << std::endl;
    //delete client;
  } else {
    std::cout << "arf" << std::endl;
  }
  delete sock;
#if defined(_WIN32)
  WSACleanup();
#endif
}
