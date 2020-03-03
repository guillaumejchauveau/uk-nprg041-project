#include "net/tcp.h"
#include <iostream>

int main() {
  net::SocketInitializer socket_initializer;

  auto server = net::TCPServer::with(AF_INET, nullptr, "8080");
  server->listen();
  server->run();
}
